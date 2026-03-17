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

#include "verible/verilog/analysis/checkers/proprietary-notice-check-rule.h"

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
VERILOG_REGISTER_LINT_RULE(ProprietaryNoticeCheckRule);

static constexpr std::string_view kRequiredNotice = "PROPRIETARY NOTICE";
static constexpr std::string_view kMessage =
    "File must contain \"PROPRIETARY NOTICE\" in the comments.";

const LintRuleDescriptor &ProprietaryNoticeCheckRule::GetDescriptor() {
  static const LintRuleDescriptor d{
      .name = "proprietary-notice-check",
      .topic = "copyright",
      .desc =
          "Checks that every file contains the text \"PROPRIETARY NOTICE\" "
          "in the comments. This is typically required for proprietary or "
          "confidential source code files.",
  };
  return d;
}

void ProprietaryNoticeCheckRule::Lint(const TextStructureView &text_structure,
                                      std::string_view) {
  found_notice_ = false;

  // Iterate through all lines to check for comments containing the notice
  for (const auto &line : text_structure.Lines()) {
    std::string_view line_view = line;

    // Check if this is a comment line (starts with //)
    if (absl::StartsWith(line_view, "//")) {
      // Check if the line contains "PROPRIETARY NOTICE" (case-sensitive)
      if (absl::StrContains(line_view, kRequiredNotice)) {
        found_notice_ = true;
        break;
      }
    }
  }

  // Also check in block comments (/* */)
  // Scan the entire file content for block comments
  std::string_view contents = text_structure.Contents();
  size_t pos = 0;
  while (pos < contents.size()) {
    // Find start of block comment
    size_t block_start = contents.find("/*", pos);
    if (block_start == std::string_view::npos) {
      break;
    }

    // Find end of block comment
    size_t block_end = contents.find("*/", block_start + 2);
    if (block_end == std::string_view::npos) {
      // Unclosed block comment, check what we have
      block_end = contents.size();
    } else {
      block_end += 2;  // Include the closing */
    }

    std::string_view block_comment =
        contents.substr(block_start, block_end - block_start);
    if (absl::StrContains(block_comment, kRequiredNotice)) {
      found_notice_ = true;
      break;
    }

    pos = block_end;
  }

  // If notice not found, report violation at the end of the first line
  if (!found_notice_) {
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

absl::Status ProprietaryNoticeCheckRule::Configure(
    std::string_view configuration) {
  // No configuration needed for this rule
  return absl::OkStatus();
}

LintRuleStatus ProprietaryNoticeCheckRule::Report() const {
  return LintRuleStatus(violations_, GetDescriptor());
}

}  // namespace analysis
}  // namespace verilog
