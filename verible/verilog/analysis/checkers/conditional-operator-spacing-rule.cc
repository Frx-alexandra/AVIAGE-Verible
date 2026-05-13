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

#include "verible/verilog/analysis/checkers/conditional-operator-spacing-rule.h"

#include <set>
#include <string_view>

#include "absl/strings/str_cat.h"
#include "verible/common/analysis/lint-rule-status.h"
#include "verible/common/text/text-structure.h"
#include "verible/verilog/analysis/descriptions.h"
#include "verible/verilog/analysis/lint-rule-registry.h"
#include "verible/verilog/parser/verilog-token-enum.h"

namespace verilog {
namespace analysis {

using verible::LintRuleStatus;
using verible::LintViolation;
using verible::TextStructureView;

// Register the lint rule
VERILOG_REGISTER_LINT_RULE(ConditionalOperatorSpacingRule);

static constexpr std::string_view kMessage =
    "Conditional/comparison operators should have equal spacing on both sides. ";

const LintRuleDescriptor &ConditionalOperatorSpacingRule::GetDescriptor() {
  static const LintRuleDescriptor d{
      .name = "conditional-operator-spacing",
      .topic = "spacing",
      .desc =
          "Checks that conditional and comparison operators (==, !=, <, >, "
          "<=, >=, ===, !==) have equal spacing on both sides. This ensures "
          "consistent formatting around these operators. Either no spaces or "
          "one space on each side is acceptable, but the spacing must be "
          "symmetric.",
  };
  return d;
}

// Simple helper to check if a character is an operator character that can
// form comparison operators.
static bool IsOpChar(char c) {
  return c == '=' || c == '!' || c == '<' || c == '>';
}

// Checks a single line for conditional/comparison operators and validates
// that spacing on both sides is symmetric.
static void CheckLineForOperatorSpacing(std::string_view line,
                                        std::set<LintViolation> *violations) {
  if (line.empty()) return;

  const char *base = line.data();
  const char *end = base + line.size();
  const char *p = base;

  while (p < end) {
    // Skip non-operator characters.
    if (!IsOpChar(*p)) {
      ++p;
      continue;
    }

    const char *op_start = p;
    // Consume consecutive operator chars to get the full operator token,
    // e.g. "==", "!=", "<=", ">=", "===", "!==".
    // Limit to maximum 3 characters (for "===" and "!==")
    while (p < end && IsOpChar(*p) && (p - op_start) < 3) ++p;
    const char *op_end = p;  // one past last operator char

    const std::string_view op_text(op_start, op_end - op_start);

    // Only care about our comparison/conditional operators.
    if (!(op_text == "==" || op_text == "!=" || op_text == "<" ||
          op_text == ">" || op_text == "<=" || op_text == ">=" ||
          op_text == "===" || op_text == "!==")) {
      continue;
    }

    // Look left for spaces until the previous non-space or beginning.
    int spaces_before = 0;
    const char *left = op_start;
    while (left > base && (*(left - 1) == ' ' || *(left - 1) == '\t')) {
      --left;
      ++spaces_before;
    }

    // If there is no non-space before, we don't enforce spacing.
    if (left == base) continue;

    // Look right for spaces until the next non-space or end.
    int spaces_after = 0;
    const char *right = op_end;
    while (right < end && (*right == ' ' || *right == '\t')) {
      ++right;
      ++spaces_after;
    }

    // If there is no non-space after, we don't enforce spacing.
    if (right == end) continue;

    if (spaces_before != spaces_after) {
      const std::string message = absl::StrCat(
          kMessage, "Found ", spaces_before, " space(s) before and ",
          spaces_after, " space(s) after '", op_text, "'.");
      // Use just the operator text for reporting.
      const verible::TokenInfo token(TK_OTHER, op_text);
      violations->insert(LintViolation(token, message));
    }
  }
}

void ConditionalOperatorSpacingRule::Lint(const TextStructureView &text_structure,
                                          std::string_view) {
  const auto &lines = text_structure.Lines();
  for (const auto &line : lines) {
    CheckLineForOperatorSpacing(line, &violations_);
  }
}

verible::LintRuleStatus ConditionalOperatorSpacingRule::Report() const {
  return LintRuleStatus(violations_, GetDescriptor());
}

}  // namespace analysis
}  // namespace verilog
