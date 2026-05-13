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

#include "verible/verilog/analysis/checkers/wire-reg-declaration-alignment-rule.h"

#include <algorithm>
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
VERILOG_REGISTER_LINT_RULE(WireRegDeclarationAlignmentRule);

static constexpr std::string_view kMessage =
    "Wire/reg declaration does not match the first declaration style in this file. ";

const LintRuleDescriptor &WireRegDeclarationAlignmentRule::GetDescriptor() {
  static const LintRuleDescriptor d{
      .name = "wire-reg-declaration-alignment",
      .topic = "formatting",
      .desc =
          "Checks that wire and reg declarations: (1) start from the first byte "
          "of the line (column 0) with no leading whitespace, and (2) follow "
          "consistent alignment for range/dimension start, signal name start, "
          "and trailing semicolon position. The alignment style follows the first "
          "declaration in the file.",
  };
  return d;
}

// Helper function to trim leading and trailing whitespace
static std::string_view Trim(std::string_view str) {
  const char *begin = str.data();
  const char *end = begin + str.size();
  
  // Trim leading whitespace
  while (begin < end && (*begin == ' ' || *begin == '\t')) {
    ++begin;
  }
  
  // Trim trailing whitespace (including newlines)
  while (end > begin && (*(end - 1) == ' ' || *(end - 1) == '\t' || 
                         *(end - 1) == '\n' || *(end - 1) == '\r')) {
    --end;
  }
  
  return std::string_view(begin, end - begin);
}

// Helper function to check if a line contains a wire or reg declaration
static bool IsWireOrRegDeclaration(std::string_view line) {
  std::string_view trimmed = Trim(line);
  
  // Check if line starts with "wire" or "reg" (as a keyword)
  if (trimmed.size() < 4) return false;
  
  if (trimmed.substr(0, 4) == "wire" || 
      (trimmed.size() >= 3 && trimmed.substr(0, 3) == "reg")) {
    // Make sure it's followed by whitespace or '['
    size_t keyword_len = (trimmed.substr(0, 4) == "wire") ? 4 : 3;
    if (trimmed.size() > keyword_len) {
      char next_char = trimmed[keyword_len];
      return next_char == ' ' || next_char == '\t' || next_char == '[';
    }
    return true;
  }
  
  return false;
}

bool WireRegDeclarationAlignmentRule::ParseDeclaration(
    std::string_view line, size_t line_number, DeclarationInfo *info) {
  if (!IsWireOrRegDeclaration(line)) {
    return false;
  }
  
  info->line_number = line_number;
  info->line_text = line;
  
  const char *p = line.data();
  const char *end = p + line.size();
  
  // Calculate indentation (leading whitespace)
  const char *start = p;
  while (p < end && (*p == ' ' || *p == '\t')) ++p;
  info->indentation = p - start;
  
  // Skip the keyword
  if (end - p >= 4 && std::string_view(p, 4) == "wire") {
    p += 4;
  } else if (end - p >= 3 && std::string_view(p, 3) == "reg") {
    p += 3;
  } else {
    return false;
  }
  
  info->keyword_end = p - line.data();
  
  // Skip whitespace after keyword
  while (p < end && (*p == ' ' || *p == '\t')) ++p;
  
  // Check for range/dimension [...]
  if (p < end && *p == '[') {
    info->has_range = true;
    info->range_start = p - line.data();
    
    // Skip to the end of the range
    int bracket_depth = 0;
    while (p < end) {
      if (*p == '[') ++bracket_depth;
      else if (*p == ']') {
        --bracket_depth;
        if (bracket_depth == 0) {
          ++p;
          break;
        }
      }
      ++p;
    }
    
    // Skip whitespace after range
    while (p < end && (*p == ' ' || *p == '\t')) ++p;
  } else {
    info->has_range = false;
    info->range_start = 0;  // Not used when has_range is false
  }
  
  // Now we should be at the signal name
  if (p >= end || (!std::isalnum(*p) && *p != '_')) {
    return false;
  }
  
  info->name_start = p - line.data();
  
  // Find the semicolon
  const char *semicolon = p;
  while (semicolon < end && *semicolon != ';') ++semicolon;
  
  if (semicolon >= end) {
    return false;  // No semicolon found
  }
  
  info->semicolon_pos = semicolon - line.data();
  
  return true;
}

void WireRegDeclarationAlignmentRule::CheckAlignment(
    const DeclarationInfo &info) {
  // FIRST: Check if declaration starts at column 0 (new requirement)
  if (info.indentation > 0) {
    std::string message = absl::StrCat(
        "Wire/reg declaration must start from the first byte of the line (column 0). ",
        "Found ", info.indentation, " byte(s) of leading whitespace.");
    
    // Report from start of keyword to end (excluding semicolon)
    std::string_view violation_text = info.line_text.substr(
        info.indentation, info.semicolon_pos - info.indentation);
    TokenInfo token(TK_OTHER, violation_text);
    violations_.insert(LintViolation(token, message));
    // Note: We continue checking alignment even if column 0 check fails
  }
  
  // SECOND: Check alignment consistency (original requirement)
  if (!expected_style_.initialized) {
    // First declaration - set name and semicolon alignment from this
    expected_style_.initialized = true;
    expected_style_.first_decl_line = info.line_number + 1;  // +1 for 1-indexed line numbers
    expected_style_.keyword_end = info.keyword_end;
    expected_style_.name_start = info.name_start;
    expected_style_.semicolon_pos = info.semicolon_pos;
    
    // If first declaration has a range, also set range alignment
    if (info.has_range) {
      expected_style_.has_range_style = true;
      expected_style_.first_range_line = info.line_number + 1;
      expected_style_.range_start = info.range_start;
    }
    return;
  }
  
  // If this declaration has a range but we haven't set range style yet,
  // this is the first declaration with a range - set the range alignment
  if (info.has_range && !expected_style_.has_range_style) {
    expected_style_.has_range_style = true;
    expected_style_.first_range_line = info.line_number + 1;
    expected_style_.range_start = info.range_start;
  }
  
  // Check alignment against expected style
  bool has_violation = false;
  std::string violation_details;
  // Start from range if present, otherwise from name
  size_t violation_start = info.has_range ? info.range_start : info.name_start;
  
  // Check range alignment if this declaration has a range AND we have a range style set
  if (info.has_range && expected_style_.has_range_style) {
    if (info.range_start != expected_style_.range_start) {
      has_violation = true;
      violation_details += absl::StrCat(
          "Range/dimension should start at column ", expected_style_.range_start,
          " (as defined by first declaration with range on line ", 
          expected_style_.first_range_line,
          "), but starts at ", info.range_start, ". ");
    }
  }
  
  if (info.name_start != expected_style_.name_start) {
    has_violation = true;
    violation_details += absl::StrCat(
        "Signal name should start at column ", expected_style_.name_start,
        " (as defined by first declaration on line ", expected_style_.first_decl_line,
        "), but starts at ", info.name_start, ". ");
  }
  
  if (info.semicolon_pos != expected_style_.semicolon_pos) {
    has_violation = true;
    violation_details += absl::StrCat(
        "Semicolon should be at column ", expected_style_.semicolon_pos,
        " (as defined by first declaration on line ", expected_style_.first_decl_line,
        "), but is at ", info.semicolon_pos, ". ");
  }
  
  if (has_violation) {
    std::string message = absl::StrCat(
        "Wire/reg declaration does not match the first declaration style in this file. ",
        violation_details);
    // Report violation from the violation start point up to (but not including) the semicolon
    std::string_view violation_text = info.line_text.substr(
        violation_start, info.semicolon_pos - violation_start);
    TokenInfo token(TK_OTHER, violation_text);
    violations_.insert(LintViolation(token, message));
  }
}

void WireRegDeclarationAlignmentRule::Lint(
    const TextStructureView &text_structure, std::string_view) {
  const auto &lines = text_structure.Lines();
  
  for (size_t i = 0; i < lines.size(); ++i) {
    DeclarationInfo info;
    if (ParseDeclaration(lines[i], i, &info)) {
      CheckAlignment(info);
    }
  }
}

LintRuleStatus WireRegDeclarationAlignmentRule::Report() const {
  return LintRuleStatus(violations_, GetDescriptor());
}

}  // namespace analysis
}  // namespace verilog
