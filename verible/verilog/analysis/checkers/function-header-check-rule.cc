// Copyright 2017-2023 The Verible Authors.
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

#include "verible/verilog/analysis/checkers/function-header-check-rule.h"

#include <cctype>
#include <set>
#include <string>
#include <string_view>
#include <vector>

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
VERILOG_REGISTER_LINT_RULE(FunctionHeaderCheckRule);

static constexpr std::string_view kRequiredPattern = "FUNCTION";
static constexpr std::string_view kMessage =
    "Function must have \"FUNCTION\" in the comments before the "
    "function definition.";

// Helper function for case-insensitive string matching
static bool CaseInsensitiveContains(std::string_view text,
                                    std::string_view pattern) {
  if (pattern.empty()) return true;
  if (text.size() < pattern.size()) return false;

  std::string text_lower;
  text_lower.reserve(text.size());
  for (char c : text) {
    text_lower.push_back(std::tolower(static_cast<unsigned char>(c)));
  }

  std::string pattern_lower;
  pattern_lower.reserve(pattern.size());
  for (char c : pattern) {
    pattern_lower.push_back(std::tolower(static_cast<unsigned char>(c)));
  }

  return text_lower.find(pattern_lower) != std::string::npos;
}

const LintRuleDescriptor &FunctionHeaderCheckRule::GetDescriptor() {
  static const LintRuleDescriptor d{
      .name = "function-header-check",
      .topic = "comments",
      .desc =
          "Checks that every function has the text \"FUNCTION\" "
          "in the comments before the function definition. This is typically "
          "required for documenting functions.",
  };
  return d;
}

// Split contents into lines
static std::vector<std::string_view> SplitLines(std::string_view contents) {
  std::vector<std::string_view> lines;
  size_t start = 0;
  size_t end = 0;

  while (start < contents.size()) {
    end = contents.find('\n', start);
    if (end == std::string_view::npos) {
      lines.push_back(contents.substr(start));
      break;
    } else {
      lines.push_back(contents.substr(start, end - start));
      start = end + 1;
    }
  }

  return lines;
}

void FunctionHeaderCheckRule::Lint(const TextStructureView &text_structure,
                                   std::string_view) {
  std::string_view contents = text_structure.Contents();
  std::vector<std::string_view> lines = SplitLines(contents);

  // Find all function definitions and check if they have FUNCTION comment
  // before them
  for (size_t line_idx = 0; line_idx < lines.size(); ++line_idx) {
    std::string_view line = lines[line_idx];

    // Look for "function" keyword on this line
    size_t func_pos = line.find("function");
    if (func_pos == std::string_view::npos) {
      continue;
    }

    // Check if "function" appears before a comment on this line
    size_t comment_pos = line.find("//");
    if (comment_pos != std::string_view::npos && func_pos > comment_pos) {
      // "function" is inside a comment, skip
      continue;
    }

    // Check for block comment /* */ that might contain "function"
    size_t block_start = line.find("/*");
    size_t block_end = line.find("*/");
    if (block_start != std::string_view::npos &&
        block_end != std::string_view::npos && block_start < func_pos &&
        func_pos < block_end) {
      // "function" is inside a block comment, skip
      continue;
    }

    // Verify it's a standalone "function" keyword (followed by whitespace or
    // special char)
    size_t after_func = func_pos + 8;
    if (after_func < line.size()) {
      char next_char = line[after_func];
      if (next_char != ' ' && next_char != '\t' && next_char != '(' &&
          next_char != ';' && next_char != '\r') {
        continue;
      }
    }

    // Verify it's not part of a larger word (preceded by whitespace or start of
    // line)
    if (func_pos > 0) {
      char prev_char = line[func_pos - 1];
      if (prev_char != ' ' && prev_char != '\t') {
        continue;
      }
    }

    // Check if there's a FUNCTION comment on the SAME line after the function
    // declaration
    if (comment_pos != std::string_view::npos && comment_pos > func_pos) {
      std::string_view comment_text = line.substr(comment_pos + 2);
      if (absl::StrContains(comment_text, kRequiredPattern)) {
        // FUNCTION comment on same line after function declaration - valid
        continue;
      }
    }

    // Now search backwards for FUNCTION comment
    bool found_function_header = false;

    // Search up to 10 lines backwards (to avoid searching entire file)
    size_t search_start = (line_idx > 10) ? line_idx - 10 : 0;
    for (size_t search_idx = line_idx; search_idx > search_start;
         --search_idx) {
      std::string_view prev_line = lines[search_idx - 1];

      // Skip blank lines
      bool is_blank = true;
      for (char c : prev_line) {
        if (c != ' ' && c != '\t' && c != '\r') {
          is_blank = false;
          break;
        }
      }

      if (is_blank) {
        continue;
      }

      // Check for // comment with FUNCTION
      size_t comment_start = prev_line.find("//");
      if (comment_start != std::string_view::npos) {
        std::string_view comment_text = prev_line.substr(comment_start + 2);
        if (CaseInsensitiveContains(comment_text, kRequiredPattern)) {
          found_function_header = true;
          break;
        }
      }

      // Check for /* */ block comment with FUNCTION
      size_t block_start = prev_line.find("/*");
      if (block_start != std::string_view::npos) {
        size_t block_end = prev_line.find("*/", block_start + 2);
        if (block_end != std::string_view::npos) {
          std::string_view block_comment =
              prev_line.substr(block_start, block_end - block_start + 2);
          if (CaseInsensitiveContains(block_comment, kRequiredPattern)) {
            found_function_header = true;
            break;
          }
        }
      }

      // If this is a non-blank line without FUNCTION, stop searching
      break;
    }

    if (!found_function_header) {
      // Report violation - calculate position in original text
      size_t violation_pos = 0;
      for (size_t i = 0; i < line_idx; ++i) {
        violation_pos += lines[i].size() + 1;  // +1 for newline
      }
      violation_pos += func_pos;

      const TokenInfo token(TK_OTHER, contents.substr(violation_pos, 8));
      violations_.insert(LintViolation(token, kMessage));
    }
  }
}

absl::Status FunctionHeaderCheckRule::Configure(
    std::string_view configuration) {
  // No configuration needed for this rule
  return absl::OkStatus();
}

LintRuleStatus FunctionHeaderCheckRule::Report() const {
  return LintRuleStatus(violations_, GetDescriptor());
}

}  // namespace analysis
}  // namespace verilog
