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

#include "verible/verilog/analysis/checkers/one-statement-per-line-rule.h"

#include <string_view>

#include "verible/common/analysis/lint-rule-status.h"
#include "verible/common/text/token-info.h"
#include "verible/verilog/analysis/descriptions.h"
#include "verible/verilog/analysis/lint-rule-registry.h"
#include "verible/verilog/parser/verilog-token-enum.h"

namespace verilog {
namespace analysis {

using verible::LintRuleStatus;
using verible::LintViolation;
using verible::TokenInfo;

// Register OneStatementPerLineRule.
VERILOG_REGISTER_LINT_RULE(OneStatementPerLineRule);

static constexpr std::string_view kMessage =
    "each statement should begin on a new line";

const LintRuleDescriptor &OneStatementPerLineRule::GetDescriptor() {
  static const LintRuleDescriptor d{
      .name = "one-statement-per-line",
      .topic = "statement-formatting",
      .desc =
          "Check that each statement begins on a new line. "
          "Multiple statements on the same line should be split into "
          "separate lines.",
  };
  return d;
}

void OneStatementPerLineRule::HandleToken(const TokenInfo &token) {
  int token_enum = token.token_enum();

  // Track newlines
  if (token_enum == TK_NEWLINE) {
    seen_newline_ = true;
    violation_reported_on_line_ = false;
    return;
  }

  // Skip whitespace and comments
  switch (token_enum) {
    case TK_SPACE:
    case TK_COMMENT_BLOCK:
    case TK_EOL_COMMENT:
      return;
    default:
      break;
  }

  // Track parentheses and brackets
  if (token_enum == '(') {
    paren_depth_++;
  } else if (token_enum == ')') {
    paren_depth_--;
  } else if (token_enum == '[') {
    bracket_depth_++;
  } else if (token_enum == ']') {
    bracket_depth_--;
  } else if (token_enum == '?') {
    // Track ? for ternary operator (increment for nesting)
    question_mark_depth_++;
  }

  // Detect control structures that expect conditions
  if (token_enum == TK_if || token_enum == TK_for || token_enum == TK_while ||
      token_enum == TK_foreach) {
    after_condition_ = true;
    in_case_statement_ = false;
    seen_newline_ = false;
  } else if (token_enum == TK_case || token_enum == TK_casex ||
             token_enum == TK_casez) {
    // Case statements are special - after condition, we get case labels
    after_condition_ = true;
    in_case_statement_ = true;
    seen_newline_ = false;
  } else if (token_enum == TK_begin) {
    // After begin, expect statements (but not labels like begin : name)
    after_begin_ = true;
    // Reset all other context flags when we see begin
    after_condition_ = false;
    after_else_ = false;
    after_assign_ = false;
    after_case_label_ = false;
    after_semicolon_ = false;
    in_case_statement_ = false;
    seen_newline_ = false;
    // Reset after_begin_ if we see a colon (begin : label)
    // The label after colon should not be flagged
    saw_begin_ = true;
  } else if (token_enum == TK_else) {
    // After else, expect statement (could be if or regular statement)
    after_else_ = true;
    in_case_statement_ = false;
    seen_newline_ = false;
  } else if (token_enum == TK_assign) {
    // After assign, we might have another statement after semicolon
    after_assign_ = true;
    in_case_statement_ = false;
    seen_newline_ = false;
  } else if (token_enum == ':') {
    // After colon (case label), expect statement
    // But not if we're inside brackets (array dimensions like [a:b])
    // And not if we saw ? (ternary operator ?:)
    if (potential_case_label_ && bracket_depth_ == 0 &&
        question_mark_depth_ == 0) {
      after_case_label_ = true;
      seen_newline_ = false;
    }
    // Decrement question_mark_depth_ when we see : (matches with ?)
    if (question_mark_depth_ > 0) {
      question_mark_depth_--;
    }
    // Handle begin : label - the label is not a statement
    if (saw_begin_) {
      after_begin_ = false;
      saw_begin_ = false;
      // Also clear after_condition_ to be safe
      after_condition_ = false;
    }
  } else if (token_enum == SymbolIdentifier || token_enum == TK_DecNumber ||
             token_enum == TK_UnBasedNumber || token_enum == TK_default) {
    // Check if this could be a case label
    potential_case_label_ = true;

    // Check for violations - only if not inside parentheses and not a case
    // label itself
    if (!violation_reported_on_line_ && paren_depth_ == 0 &&
        !after_case_label_) {
      if (after_condition_ && !seen_newline_ && !in_case_statement_) {
        // Statement after condition on same line (if, for, while, etc.)
        violations_.insert(LintViolation(token, kMessage));
        violation_reported_on_line_ = true;
      } else if (after_begin_ && !seen_newline_) {
        // Statement after begin on same line
        violations_.insert(LintViolation(token, kMessage));
        violation_reported_on_line_ = true;
      } else if (after_case_label_ && !seen_newline_) {
        // Statement after case label on same line
        violations_.insert(LintViolation(token, kMessage));
        violation_reported_on_line_ = true;
      } else if (after_semicolon_ && !seen_newline_) {
        // Statement after semicolon on same line (multiple statements)
        violations_.insert(LintViolation(token, kMessage));
        violation_reported_on_line_ = true;
      }
    }

    // Clear context flags only if not inside parentheses
    if (paren_depth_ == 0) {
      after_condition_ = false;
      after_begin_ = false;
      after_else_ = false;
      after_assign_ = false;
    }
    // Note: after_case_label_ is cleared after we see the identifier that
    // follows it
  } else {
    // Not an identifier, clear case label potential and saw_begin
    potential_case_label_ = false;
    saw_begin_ = false;

    // Check for else-if pattern
    if (token_enum == TK_if && after_else_) {
      after_else_ = false;
      after_condition_ = true;
    }
  }

  // Handle case label: clear after_case_label_ after we see the identifier
  // that follows it
  if (token_enum == SymbolIdentifier && after_case_label_ &&
      paren_depth_ == 0) {
    // This is the statement after the case label - check if it's a violation
    if (!violation_reported_on_line_ && !seen_newline_) {
      violations_.insert(LintViolation(token, kMessage));
      violation_reported_on_line_ = true;
    }
    after_case_label_ = false;
  }

  // Handle semicolon: check for multiple statements separated by semicolons
  if (token_enum == ';' && !violation_reported_on_line_) {
    // After a semicolon, if there's no newline, the next identifier
    // that starts a statement is a violation
    if (!seen_newline_) {
      after_semicolon_ = true;
    }
    // Clear other context flags
    after_condition_ = false;
    after_begin_ = false;
    after_else_ = false;
    after_assign_ = false;
    after_case_label_ = false;
    in_case_statement_ = false;
  }
}

LintRuleStatus OneStatementPerLineRule::Report() const {
  return LintRuleStatus(violations_, GetDescriptor());
}

}  // namespace analysis
}  // namespace verilog
