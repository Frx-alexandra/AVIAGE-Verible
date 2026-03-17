// Copyright 2017-2020 The Verible Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "verible/verilog/analysis/checkers/module-description-check-rule.h"

#include <set>
#include <string>
#include <string_view>

#include "absl/strings/match.h"
#include "verible/common/analysis/lint-rule-status.h"
#include "verible/common/text/text-structure.h"
#include "verible/common/text/token-info.h"
#include "verible/verilog/analysis/descriptions.h"
#include "verible/verilog/analysis/lint-rule-registry.h"
#include "verible/verilog/parser/verilog-token-enum.h"

namespace verilog {
namespace analysis {

using verible::LintRuleStatus;
using verible::LintViolation;
using verible::TextStructureView;
using verible::TokenInfo;

// Register the lint rule
VERILOG_REGISTER_LINT_RULE(ModuleDescriptionCheckRule);

static constexpr std::string_view kRequiredPattern = "Module Name:";
static constexpr std::string_view kMessage =
    "File must contain \"Module Name:\" in the comments before any "
    "module definition.";

const LintRuleDescriptor &ModuleDescriptionCheckRule::GetDescriptor() {
  static const LintRuleDescriptor d{
      .name = "module-description-check",
      .topic = "copyright",
      .desc =
          "Checks that every file contains the text \"Module Name:\" "
          "in the comments before any module definition. This is typically "
          "required for documenting module files.",
  };
  return d;
}

void ModuleDescriptionCheckRule::Lint(const TextStructureView &text_structure,
                                      std::string_view) {
  std::string_view contents = text_structure.Contents();

  // Find the position of Module Name:
  size_t notice_pos = std::string_view::npos;

  // Check in single-line comments
  size_t pos = 0;
  while (pos < contents.size()) {
    // Find next // comment
    size_t comment_start = contents.find("//", pos);
    if (comment_start == std::string_view::npos) {
      break;
    }

    // Find end of this line
    size_t line_end = contents.find('\n', comment_start);
    if (line_end == std::string_view::npos) {
      line_end = contents.size();
    }

    // Extract the comment text (excluding //)
    std::string_view comment_text =
        contents.substr(comment_start + 2, line_end - comment_start - 2);
    if (absl::StrContains(comment_text, kRequiredPattern)) {
      notice_pos = comment_start;
      break;
    }

    pos = line_end + 1;
  }

  // Check in block comments if not found yet
  if (notice_pos == std::string_view::npos) {
    pos = 0;
    while (pos < contents.size()) {
      size_t block_start = contents.find("/*", pos);
      if (block_start == std::string_view::npos) {
        break;
      }

      size_t block_end = contents.find("*/", block_start + 2);
      if (block_end == std::string_view::npos) {
        block_end = contents.size();
      } else {
        block_end += 2;
      }

      std::string_view block_comment =
          contents.substr(block_start, block_end - block_start);
      if (absl::StrContains(block_comment, kRequiredPattern)) {
        notice_pos = block_start;
        break;
      }

      pos = block_end;
    }
  }

  // Find the position of the first module definition
  size_t module_pos = std::string_view::npos;

  // Look for "module" keyword that's not in a comment
  pos = 0;
  while (pos < contents.size()) {
    size_t module_keyword = contents.find("module", pos);
    if (module_keyword == std::string_view::npos) {
      break;
    }

    // Check if this "module" is inside a string or comment
    // Simple check: see if it's preceded by // on the same line
    size_t line_start = contents.rfind('\n', module_keyword);
    if (line_start == std::string_view::npos) {
      line_start = 0;
    } else {
      line_start += 1;  // Skip the newline
    }

    std::string_view line_before =
        contents.substr(line_start, module_keyword - line_start);

    // Check if // appears before "module" on this line
    if (line_before.find("//") == std::string_view::npos) {
      // This is a real module definition, not in a comment
      // Verify it's a module keyword (followed by whitespace or identifier)
      size_t after_module = module_keyword + 6;  // length of "module"
      if (after_module < contents.size()) {
        char next_char = contents[after_module];
        if (next_char == ' ' || next_char == '\t' || next_char == '\n' ||
            next_char == '#' || next_char == '(') {
          module_pos = module_keyword;
          break;
        }
      }
    }

    pos = module_keyword + 6;
  }

  // Check if pattern exists and is before module
  bool violation_found = false;

  if (notice_pos == std::string_view::npos) {
    // No Module Name: found at all
    violation_found = true;
  } else if (module_pos != std::string_view::npos && notice_pos > module_pos) {
    // Module Name: found but after module definition
    violation_found = true;
  }

  if (violation_found) {
    const auto &lines = text_structure.Lines();
    if (!lines.empty()) {
      std::string_view first_line = lines.front();
      // Remove trailing newline if present
      if (!first_line.empty() && first_line.back() == '\n') {
        first_line = first_line.substr(0, first_line.length() - 1);
      }
      // Point to the end of the first line (0-length substring)
      const TokenInfo token(TK_OTHER,
                            first_line.substr(first_line.length(), 0));
      violations_.insert(LintViolation(token, kMessage));
    }
  }
}

absl::Status ModuleDescriptionCheckRule::Configure(
    std::string_view configuration) {
  // No configuration needed for this rule
  return absl::OkStatus();
}

LintRuleStatus ModuleDescriptionCheckRule::Report() const {
  return LintRuleStatus(violations_, GetDescriptor());
}

}  // namespace analysis
}  // namespace verilog
