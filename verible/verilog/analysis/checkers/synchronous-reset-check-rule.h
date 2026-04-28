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

#ifndef VERIBLE_VERILOG_ANALYSIS_CHECKERS_SYNCHRONOUS_RESET_CHECK_RULE_H_
#define VERIBLE_VERILOG_ANALYSIS_CHECKERS_SYNCHRONOUS_RESET_CHECK_RULE_H_

#include <set>
#include <string_view>

#include "verible/common/analysis/lint-rule-status.h"
#include "verible/common/analysis/syntax-tree-lint-rule.h"
#include "verible/common/text/symbol.h"
#include "verible/common/text/syntax-tree-context.h"
#include "verible/verilog/analysis/descriptions.h"

namespace verilog {
namespace analysis {

// SynchronousResetCheckRule checks that all always blocks:
// 1. Have exactly one clock signal in the sensitivity list
// 2. Have a synchronous reset check in the first if condition inside the block
// Clock signals are identified by containing "clk" or "clock"
// (case-insensitive) Reset signals are identified by containing "rst" or
// "reset" (case-insensitive)
class SynchronousResetCheckRule : public verible::SyntaxTreeLintRule {
 public:
  using rule_type = verible::SyntaxTreeLintRule;
  static const LintRuleDescriptor &GetDescriptor();

  void HandleSymbol(const verible::Symbol &symbol,
                    const verible::SyntaxTreeContext &context) final;

  verible::LintRuleStatus Report() const final;

 private:
  // Checks if a string contains clock-related keywords
  static bool IsClockSignal(std::string_view signal_name);

  // Checks if a string contains reset-related keywords
  static bool IsResetSignal(std::string_view signal_name);

  // Extract signal name from event expression
  static std::string_view ExtractSignalName(const verible::Symbol &event_expr);

  // Check if the block contains a reset signal check
  static bool HasResetCheck(const verible::Symbol &always_statement);

  std::set<verible::LintViolation> violations_;
};

}  // namespace analysis
}  // namespace verilog

#endif  // VERIBLE_VERILOG_ANALYSIS_CHECKERS_SYNCHRONOUS_RESET_CHECK_RULE_H_
