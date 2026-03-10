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

#ifndef VERIBLE_VERILOG_ANALYSIS_CHECKERS_ONE_STATEMENT_PER_LINE_RULE_H_
#define VERIBLE_VERILOG_ANALYSIS_CHECKERS_ONE_STATEMENT_PER_LINE_RULE_H_

#include <set>
#include <string_view>

#include "verible/common/analysis/lint-rule-status.h"
#include "verible/common/analysis/token-stream-lint-rule.h"
#include "verible/common/text/token-info.h"
#include "verible/verilog/analysis/descriptions.h"

namespace verilog {
namespace analysis {

// OneStatementPerLineRule checks that each statement begins on a new line.
class OneStatementPerLineRule : public verible::TokenStreamLintRule {
 public:
  using rule_type = verible::TokenStreamLintRule;

  static const LintRuleDescriptor &GetDescriptor();

  OneStatementPerLineRule()
      : seen_newline_(true),
        violation_reported_on_line_(false),
        after_condition_(false),
        after_begin_(false),
        after_else_(false),
        after_assign_(false),
        after_case_label_(false),
        after_semicolon_(false),
        in_case_statement_(false),
        potential_case_label_(false),
        saw_begin_(false),
        question_mark_depth_(0),
        paren_depth_(0),
        bracket_depth_(0) {}

  void HandleToken(const verible::TokenInfo &token) final;

  verible::LintRuleStatus Report() const final;

 private:
  // State tracking
  bool seen_newline_;
  bool violation_reported_on_line_;
  bool after_condition_;
  bool after_begin_;
  bool after_else_;
  bool after_assign_;
  bool after_case_label_;
  bool after_semicolon_;
  bool in_case_statement_;
  bool potential_case_label_;
  bool saw_begin_;
  int question_mark_depth_;  // Track ? for ternary operator (handles nesting)
  int paren_depth_;
  int bracket_depth_;  // Track [] for array dimensions

  std::set<verible::LintViolation> violations_;
};

}  // namespace analysis
}  // namespace verilog

#endif  // VERIBLE_VERILOG_ANALYSIS_CHECKERS_ONE_STATEMENT_PER_LINE_RULE_H_
