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

#include "verible/verilog/analysis/checkers/multi-line-comments-rule.h"

#include <regex>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "absl/status/status.h"
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
VERILOG_REGISTER_LINT_RULE(MultiLineCommentsRule);

static constexpr std::string_view kDefaultMessage =
    "Multi-line comments must be surrounded by uniform format: // followed by "
    "58 equal signs.";

const LintRuleDescriptor &MultiLineCommentsRule::GetDescriptor() {
  static const LintRuleDescriptor d{
      .name = "multi-line-comments",
      .topic = "comments",
      .desc =
          "Checks that multi-line comments (consecutive // lines) are "
          "surrounded "
          "by a uniform format. Default: two forward slashes followed by 58 "
          "equal "
          "signs. Configurable via 'border_pattern' option.",
  };
  return d;
}

void MultiLineCommentsRule::Lint(const TextStructureView &text_structure,
                                 std::string_view) {
  // Build dynamic regex based on configured border pattern
  std::string border_pattern_escaped = std::regex_replace(
      border_pattern_, std::regex(R"([.^$|()\\[{}]?*+])"), R"(\$&)");
  std::string border_regex_str = "^//" + border_pattern_escaped + "$";
  const std::regex border_regex(border_regex_str);
  const std::regex comment_regex(R"(^//\s*.*$)");

  std::vector<size_t> border_lines;

  size_t lineno = 0;
  for (const auto &line : text_structure.Lines()) {
    std::string_view line_full = line;
    std::string_view line_stripped = line_full;
    if (absl::EndsWith(line_stripped, "\n"))
      line_stripped = line_stripped.substr(0, line_stripped.size() - 1);
    if (std::regex_match(std::string(line_stripped), border_regex)) {
      border_lines.push_back(lineno);
    }

    if (absl::StartsWith(line, "//")) {
      // Find the start of a potential multi-line comment block
      size_t start_line = lineno;
      size_t end_line = lineno;

      // Count consecutive // lines
      while (end_line < text_structure.Lines().size() &&
             absl::StartsWith(text_structure.Lines()[end_line], "//")) {
        ++end_line;
      }

      size_t block_size = end_line - start_line;

      if (block_size > 1) {  // Multi-line comment block
        // Check if proper
        std::string_view first_full = text_structure.Lines()[start_line];
        std::string_view first_stripped = first_full;
        if (absl::EndsWith(first_stripped, "\n"))
          first_stripped = first_stripped.substr(0, first_stripped.size() - 1);
        bool first_is_border =
            std::regex_match(std::string(first_stripped), border_regex);

        std::string_view last_full = text_structure.Lines()[end_line - 1];
        std::string_view last_stripped = last_full;
        if (absl::EndsWith(last_stripped, "\n"))
          last_stripped = last_stripped.substr(0, last_stripped.size() - 1);
        bool last_is_border =
            std::regex_match(std::string(last_stripped), border_regex);

        if (!(first_is_border && last_is_border)) {
          // violation only on the last line of multi-line comment block
          std::string violation_msg =
              "Multi-line comments must be surrounded by uniform format: " +
              format_message_ + ".";
          size_t last_violation_line = end_line - 1;
          std::string_view last_full =
              text_structure.Lines()[last_violation_line];
          TokenInfo token(TK_OTHER, last_full);
          violations_.insert(LintViolation(token, violation_msg));
        }
      }

      // Skip the lines we've processed
      lineno = end_line - 1;
    }
    ++lineno;
  }

  // Check between borders
  for (size_t i = 0; i + 1 < border_lines.size(); ++i) {
    size_t start = border_lines[i] + 1;
    size_t end = border_lines[i] + 1;
    for (size_t j = start; j < end; ++j) {
      std::string_view j_full = text_structure.Lines()[j];
      std::string_view j_stripped = j_full;
      if (absl::EndsWith(j_stripped, "\n"))
        j_stripped = j_stripped.substr(0, j_stripped.size() - 1);
      if (!std::regex_match(std::string(j_stripped), comment_regex)) {
        std::string violation_msg =
            "Multi-line comments must be surrounded by uniform format: // "
            "followed by " +
            format_message_ + ".";
        TokenInfo token(TK_OTHER, j_full);
        violations_.insert(LintViolation(token, violation_msg));
      }
    }
  }
}

absl::Status MultiLineCommentsRule::Configure(std::string_view configuration) {
  if (configuration.empty()) {
    return absl::OkStatus();
  }

  // Parse JSON-like configuration: {"border_pattern": "===...==="}
  // Simple parsing for border_pattern key
  std::string config_str(configuration);

  // Look for border_pattern in configuration
  std::string key = "\"border_pattern\"";
  size_t key_pos = config_str.find(key);
  if (key_pos != std::string::npos) {
    size_t colon_pos = config_str.find(":", key_pos);
    if (colon_pos != std::string::npos) {
      size_t start_quote = config_str.find("\"", colon_pos);
      if (start_quote != std::string::npos) {
        size_t end_quote = config_str.find("\"", start_quote + 1);
        if (end_quote != std::string::npos) {
          border_pattern_ =
              config_str.substr(start_quote + 1, end_quote - start_quote - 1);
          format_message_ = border_pattern_ + " (user-defined)";
        }
      }
    }
  }

  return absl::OkStatus();
}

LintRuleStatus MultiLineCommentsRule::Report() const {
  return LintRuleStatus(violations_, GetDescriptor());
}

}  // namespace analysis
}  // namespace verilog