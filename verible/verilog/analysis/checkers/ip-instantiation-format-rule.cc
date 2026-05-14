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

#include "verible/verilog/analysis/checkers/ip-instantiation-format-rule.h"

#include <set>
#include <string>
#include <string_view>

#include "absl/strings/str_cat.h"
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
VERILOG_REGISTER_LINT_RULE(IPInstantiationFormatRule);

static constexpr std::string_view kMessage =
    "IP instantiation port connection does not match the first connection format in this file. ";

const LintRuleDescriptor &IPInstantiationFormatRule::GetDescriptor() {
  static const LintRuleDescriptor d{
      .name = "ip-instantiation-format",
      .topic = "formatting",
      .desc =
          "Checks that all IP/module instantiations within a file follow "
          "consistent formatting for port connections. Specifically checks "
          "that the positions of opening and closing parentheses for each "
          "port connection (.port_name(signal)) are aligned consistently. "
          "The format is established by the first port connection line in the first "
          "instance found in the file, and all subsequent port connections must match.",
  };
  return d;
}

// Helper function to check if a line contains a port connection pattern
// Port connections look like: .port_name(signal) or .port_name  (signal)
bool IPInstantiationFormatRule::IsPortConnectionLine(std::string_view line) {
  // Look for pattern: . followed by identifier followed by (
  // This is a simple heuristic - more sophisticated parsing could be done
  size_t dot_pos = line.find('.');
  if (dot_pos == std::string_view::npos) {
    return false;
  }
  
  // Find the opening parenthesis after the dot
  size_t paren_pos = line.find('(', dot_pos);
  if (paren_pos == std::string_view::npos) {
    return false;
  }
  
  // Check if there's an identifier between dot and paren
  // (at least one non-whitespace character)
  bool has_identifier = false;
  for (size_t i = dot_pos + 1; i < paren_pos; ++i) {
    if (line[i] != ' ' && line[i] != '\t') {
      has_identifier = true;
      break;
    }
  }
  
  return has_identifier;
}

bool IPInstantiationFormatRule::ParsePortConnection(
    std::string_view line, size_t line_number, PortConnectionInfo *info) {
  if (!IsPortConnectionLine(line)) {
    return false;
  }
  
  info->line_number = line_number;
  info->line_text = line;
  
  const char *p = line.data();
  const char *end = p + line.size();
  
  // Find the dot
  while (p < end && *p != '.') ++p;
  if (p >= end) return false;
  
  // Find the opening parenthesis
  const char *left_paren = p;
  while (left_paren < end && *left_paren != '(') ++left_paren;
  if (left_paren >= end) return false;
  
  info->left_paren_pos = left_paren - line.data();
  
  // Find the matching closing parenthesis
  int paren_depth = 0;
  const char *right_paren = left_paren;
  while (right_paren < end) {
    if (*right_paren == '(') {
      ++paren_depth;
    } else if (*right_paren == ')') {
      --paren_depth;
      if (paren_depth == 0) {
        break;
      }
    }
    ++right_paren;
  }
  
  if (right_paren >= end || paren_depth != 0) {
    return false;  // No matching closing paren found
  }
  
  info->right_paren_pos = right_paren - line.data();
  
  return true;
}

void IPInstantiationFormatRule::CheckFormat(const PortConnectionInfo &info) {
  if (!expected_format_.initialized) {
    // First port connection in the file - set the expected format
    expected_format_.initialized = true;
    expected_format_.first_connection_line = info.line_number + 1;  // +1 for 1-indexed
    expected_format_.left_paren_pos = info.left_paren_pos;
    expected_format_.right_paren_pos = info.right_paren_pos;
    return;
  }
  
  // Check format against expected style (file-wide)
  bool has_violation = false;
  std::string violation_details;
  
  if (info.left_paren_pos != expected_format_.left_paren_pos) {
    has_violation = true;
    violation_details += absl::StrCat(
        "Opening parenthesis should be at column ", expected_format_.left_paren_pos,
        " (as defined by first port connection on line ", 
        expected_format_.first_connection_line,
        "), but is at ", info.left_paren_pos, ". ");
  }
  
  if (info.right_paren_pos != expected_format_.right_paren_pos) {
    has_violation = true;
    violation_details += absl::StrCat(
        "Closing parenthesis should be at column ", expected_format_.right_paren_pos,
        " (as defined by first port connection on line ", 
        expected_format_.first_connection_line,
        "), but is at ", info.right_paren_pos, ". ");
  }
  
  if (has_violation) {
    std::string message = absl::StrCat(kMessage, violation_details);
    // Report violation starting from the dot (.)
    const char *line_start = info.line_text.data();
    const char *dot_pos = line_start;
    while (dot_pos < line_start + info.line_text.size() && *dot_pos != '.') {
      ++dot_pos;
    }
    if (dot_pos < line_start + info.line_text.size()) {
      std::string_view violation_text(dot_pos, 
          (line_start + info.line_text.size()) - dot_pos);
      // Trim trailing whitespace, newlines, and commas
      while (!violation_text.empty() && 
             (violation_text.back() == ' ' || violation_text.back() == '\t' ||
              violation_text.back() == '\n' || violation_text.back() == '\r' ||
              violation_text.back() == ',')) {
        violation_text.remove_suffix(1);
      }
      TokenInfo token(TK_OTHER, violation_text);
      violations_.insert(LintViolation(token, message));
    }
  }
}

void IPInstantiationFormatRule::Lint(const TextStructureView &text_structure,
                                     std::string_view) {
  const auto &lines = text_structure.Lines();
  
  for (size_t i = 0; i < lines.size(); ++i) {
    PortConnectionInfo info;
    if (ParsePortConnection(lines[i], i, &info)) {
      CheckFormat(info);
    }
  }
}

LintRuleStatus IPInstantiationFormatRule::Report() const {
  return LintRuleStatus(violations_, GetDescriptor());
}

}  // namespace analysis
}  // namespace verilog
