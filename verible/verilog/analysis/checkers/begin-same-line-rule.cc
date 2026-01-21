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

#include "verible/verilog/analysis/checkers/begin-same-line-rule.h"

#include <set>
#include <string_view>

#include "absl/base/attributes.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "verible/common/analysis/lint-rule-status.h"
#include "verible/common/analysis/token-stream-lint-rule.h"
#include "verible/common/text/config-utils.h"
#include "verible/common/text/token-info.h"
#include "verible/verilog/analysis/descriptions.h"
#include "verible/verilog/analysis/lint-rule-registry.h"
#include "verible/verilog/parser/verilog-token-enum.h"

namespace verilog {
namespace analysis {

using verible::LintRuleStatus;
using verible::LintViolation;
using verible::TokenInfo;
using verible::TokenStreamLintRule;

// Register the lint rule
VERILOG_REGISTER_LINT_RULE(BeginSameLineRule);

static const char kMessage[] =
    " begin should be on the same line with the preceding statement.";

const LintRuleDescriptor &BeginSameLineRule::GetDescriptor() {
  static const LintRuleDescriptor d{
      .name = "begin-same-line",
      .topic = "begin-same-line",
      .desc =
          "Checks that a Verilog ``begin`` directive is on the same line "
          "with the preceding always, if, or else statement.",
      .param =
          {
              {"if_enable", "true",
               "Check if statements for begin on same line"},
              {"else_enable", "true",
               "Check else statements for begin on same line"},
              {"always_enable", "true",
               "Check always statements for begin on same line"},
              {"always_comb_enable", "true",
               "Check always_comb statements for begin on same line"},
              {"always_latch_enable", "true",
               "Check always_latch statements for begin on same line"},
              {"for_enable", "true",
               "Check for statements for begin on same line"},
              {"foreach_enable", "true",
               "Check foreach statements for begin on same line"},
              {"while_enable", "true",
               "Check while statements for begin on same line"},
              {"forever_enable", "true",
               "Check forever statements for begin on same line"},
              {"initial_enable", "true",
               "Check initial statements for begin on same line"},
          },
  };
  return d;
}

absl::Status BeginSameLineRule::Configure(std::string_view configuration) {
  using verible::config::SetBool;
  return verible::ParseNameValues(
      configuration,
      {
          {"if_enable", SetBool(&if_enable_)},
          {"else_enable", SetBool(&else_enable_)},
          {"always_enable", SetBool(&always_enable_)},
          {"always_comb_enable", SetBool(&always_comb_enable_)},
          {"always_latch_enable", SetBool(&always_latch_enable_)},
          {"for_enable", SetBool(&for_enable_)},
          {"foreach_enable", SetBool(&foreach_enable_)},
          {"while_enable", SetBool(&while_enable_)},
          {"forever_enable", SetBool(&forever_enable_)},
          {"initial_enable", SetBool(&initial_enable_)},
      });
}

bool BeginSameLineRule::IsTokenEnabled(const TokenInfo &token) {
  switch (token.token_enum()) {
    case TK_if:
      return if_enable_;
    case TK_else:
      return else_enable_;
    case TK_always:
      return always_enable_;
    case TK_always_comb:
      return always_comb_enable_;
    case TK_always_latch:
      return always_latch_enable_;
    case TK_for:
      return for_enable_;
    case TK_foreach:
      return foreach_enable_;
    case TK_while:
      return while_enable_;
    case TK_forever:
      return forever_enable_;
    case TK_initial:
      return initial_enable_;
    default:
      return false;
  }
}

bool BeginSameLineRule::HandleTokenStateMachine(const TokenInfo &token) {
  bool raise_violation = false;

  bool token_has_newline = token.text().find('\n') != std::string_view::npos;
  if (token_has_newline) {
    seen_newline_since_start_ = true;
  }

  switch (state_) {
    case State::kNormal: {
      switch (token.token_enum()) {
        case TK_constraint: {
          constraint_expr_level_ = 0;
          state_ = State::kConstraint;
          return false;
        }
        case TK_with: {
          constraint_expr_level_ = 0;
          state_ = State::kInlineConstraint;
          return false;
        }
        default: {
          break;
        }
      }

      if (!IsTokenEnabled(token)) {
        break;
      }

      switch (token.token_enum()) {
        case TK_always_comb:
        case TK_always_latch:
        case TK_forever:
        case TK_initial:
          start_token_ = token;
          seen_newline_since_start_ = false;
          state_ = State::kExpectBegin;
          break;
        case TK_if:
        case TK_for:
        case TK_foreach:
        case TK_while:
          condition_expr_level_ = 0;
          start_token_ = token;
          seen_newline_since_start_ = false;
          state_ = State::kInCondition;
          break;
        case TK_always:
          condition_expr_level_ = 0;
          start_token_ = token;
          seen_newline_since_start_ = false;
          state_ = State::kInAlways;
          break;
        case TK_else:
          start_token_ = token;
          seen_newline_since_start_ = false;
          state_ = State::kInElse;
          break;
        default:
          break;
      }
      break;
    }
    case State::kInAlways: {
      switch (token.token_enum()) {
        case '@':
          break;
        case '*':
          break;
        case TK_begin:
          if (seen_newline_since_start_) {
            raise_violation = true;
          }
          state_ = State::kNormal;
          break;
        case '(':
          condition_expr_level_++;
          state_ = State::kInCondition;
          break;
        default:
          state_ = State::kNormal;
          break;
      }
      break;
    }
    case State::kInElse: {
      switch (token.token_enum()) {
        case TK_if:
          if (if_enable_) {
            condition_expr_level_ = 0;
            start_token_ = token;
            seen_newline_since_start_ = false;
            state_ = State::kInCondition;
          } else {
            state_ = State::kNormal;
          }
          break;
        case TK_begin:
          if (seen_newline_since_start_) {
            raise_violation = true;
          }
          state_ = State::kNormal;
          break;
        default:
          state_ = State::kNormal;
          break;
      }
      break;
    }
    case State::kInCondition: {
      switch (token.token_enum()) {
        case '(': {
          condition_expr_level_++;
          break;
        }
        case ')': {
          condition_expr_level_--;
          if (condition_expr_level_ == 0) {
            state_ = State::kExpectBegin;
          }
          break;
        }
        default: {
          break;
        }
      }
      break;
    }
    case State::kExpectBegin: {
      switch (token.token_enum()) {
        case TK_begin: {
          if (seen_newline_since_start_) {
            raise_violation = true;
          }
          state_ = State::kNormal;
          break;
        }
        default: {
          state_ = State::kNormal;
          break;
        }
      }
      break;
    }
    case State::kInlineConstraint: {
      switch (token.token_enum()) {
        case '{': {
          constraint_expr_level_++;
          state_ = State::kConstraint;
          break;
        }
        default: {
          state_ = State::kNormal;
          break;
        }
      }
      break;
    }
    case State::kConstraint: {
      switch (token.token_enum()) {
        case '{': {
          constraint_expr_level_++;
          break;
        }
        case '}': {
          constraint_expr_level_--;
          if (constraint_expr_level_ == 0) {
            state_ = State::kNormal;
          }
          break;
        }
        default: {
          break;
        }
      }
      break;
    }
    default: {
      break;
    }
  }

  if (raise_violation) {
    violations_.insert(LintViolation(
        start_token_, absl::StrCat(start_token_.text(), kMessage)));

    state_ = State::kNormal;
  }

  return raise_violation;
}

void BeginSameLineRule::HandleToken(const TokenInfo &token) {
  if (token.token_enum() == TK_NEWLINE) {
    seen_newline_since_start_ = true;
    return;
  }

  switch (token.token_enum()) {
    case TK_SPACE:
      return;
    case TK_COMMENT_BLOCK:
      return;
    case TK_EOL_COMMENT:
      return;
    default:
      break;
  }

  bool retry = HandleTokenStateMachine(token);

  if (retry) {
    HandleTokenStateMachine(token);
  }
}

LintRuleStatus BeginSameLineRule::Report() const {
  return LintRuleStatus(violations_, GetDescriptor());
}

}  // namespace analysis
}  // namespace verilog
