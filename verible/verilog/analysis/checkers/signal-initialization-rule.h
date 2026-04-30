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

#ifndef VERIBLE_VERILOG_ANALYSIS_CHECKERS_SIGNAL_INITIALIZATION_RULE_H_
#define VERIBLE_VERILOG_ANALYSIS_CHECKERS_SIGNAL_INITIALIZATION_RULE_H_

#include <set>
#include <string_view>

#include "verible/common/analysis/lint-rule-status.h"
#include "verible/common/analysis/syntax-tree-lint-rule.h"
#include "verible/common/text/symbol.h"
#include "verible/common/text/syntax-tree-context.h"
#include "verible/verilog/analysis/descriptions.h"

namespace verilog {
namespace analysis {

// SignalInitializationRule checks that all signals assigned with non-blocking
// assignments in always blocks with reset signals are properly initialized in
// the reset branch.
//
// The rule checks:
// 1. All always blocks that contain a reset signal (name containing 'rst' or
//    'reset') in the first if condition
// 2. All signals on the left-hand side of non-blocking assignments (<=) in
//    the always block
// 3. Each such signal must be initialized (assigned a value) in the reset
//    branch (the first if statement body)
// 4. Initialization must be to a constant or macro value
//
// Reset signals are identified by containing "rst" or "reset"
// (case-insensitive)
class SignalInitializationRule : public verible::SyntaxTreeLintRule {
 public:
  using rule_type = verible::SyntaxTreeLintRule;
  static const LintRuleDescriptor &GetDescriptor();

  void HandleSymbol(const verible::Symbol &symbol,
                    const verible::SyntaxTreeContext &context) final;

  verible::LintRuleStatus Report() const final;

 private:
  // Check if the first if statement in the always block checks a reset signal
  static bool HasResetCheckInFirstIf(const verible::Symbol &always_statement);

  // Get all signals assigned with non-blocking assignments in the always block
  static std::set<std::string_view> GetNonBlockingAssignedSignals(
      const verible::Symbol &always_statement);

  // Get all signals initialized in the reset branch
  static std::set<std::string_view> GetInitializedSignalsInResetBranch(
      const verible::Symbol &always_statement);

  // Check if an assignment is to a constant or macro
  static bool IsConstantOrMacroAssignment(
      const verible::Symbol &assignment_rhs,
      std::string_view lhs_signal_name);

  std::set<verible::LintViolation> violations_;
};

}  // namespace analysis
}  // namespace verilog

#endif  // VERIBLE_VERILOG_ANALYSIS_CHECKERS_SIGNAL_INITIALIZATION_RULE_H_
