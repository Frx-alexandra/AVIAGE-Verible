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

#ifndef VERIBLE_VERILOG_ANALYSIS_CHECKERS_BEGIN_SAME_LINE_RULE_H_
#define VERIBLE_VERILOG_ANALYSIS_CHECKERS_BEGIN_SAME_LINE_RULE_H_

#include <set>
#include <string_view>

#include "absl/status/status.h"
#include "verible/common/analysis/lint-rule-status.h"
#include "verible/common/analysis/token-stream-lint-rule.h"
#include "verible/common/text/token-info.h"
#include "verible/verilog/analysis/descriptions.h"

namespace verilog {
namespace analysis {

using verible::TokenInfo;

// Detects whether begin is on the same line with always, if, and else
// statements
class BeginSameLineRule : public verible::TokenStreamLintRule {
 public:
  using rule_type = verible::TokenStreamLintRule;

  static const LintRuleDescriptor &GetDescriptor();

  absl::Status Configure(std::string_view configuration) final;

  void HandleToken(const verible::TokenInfo &token) final;

  verible::LintRuleStatus Report() const final;

 private:
  bool HandleTokenStateMachine(const TokenInfo &token);

  bool IsTokenEnabled(const TokenInfo &token);

  // States of the internal token-based analysis.
  enum class State {
    kNormal,
    kInAlways,
    kInCondition,
    kInElse,
    kExpectBegin,
    kConstraint,
    kInlineConstraint
  };

  // Internal lexical analysis state.
  State state_{State::kNormal};

  // Level of nested parenthesis when analysing conditional expressions
  int condition_expr_level_{0};

  // Level inside a constraint expression
  int constraint_expr_level_{0};

  // Configuration
  bool if_enable_{true};
  bool else_enable_{true};
  bool always_enable_{true};
  bool always_comb_enable_{true};
  bool always_latch_enable_{true};
  bool for_enable_{true};
  bool foreach_enable_{true};
  bool while_enable_{true};
  bool forever_enable_{true};
  bool initial_enable_{true};

  // Token that requires begin on same line.
  verible::TokenInfo start_token_{verible::TokenInfo::EOFToken()};

  // Line offset of start_token_: 0 = same line, -1 = spans newline
  int start_token_line_{0};

  // Flag to track if we've seen a newline since start_token_
  bool seen_newline_since_start_{false};

  // Flag to track if we've seen a newline since start_token_
  bool seen_newline_{false};

  // Counter of tokens seen since start_token_
  int tokens_since_start_{0};

  // Collection of found violations.
  std::set<verible::LintViolation> violations_;
};

}  // namespace analysis
}  // namespace verilog

#endif  // VERIBLE_VERILOG_ANALYSIS_CHECKERS_BEGIN_SAME_LINE_RULE_H_