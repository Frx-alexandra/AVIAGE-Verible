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

#ifndef VERIBLE_VERILOG_ANALYSIS_CHECKERS_PORT_TYPE_CHECK_RULE_H_
#define VERIBLE_VERILOG_ANALYSIS_CHECKERS_PORT_TYPE_CHECK_RULE_H_

#include <set>
#include <string_view>

#include "verible/common/analysis/lint-rule-status.h"
#include "verible/common/analysis/syntax-tree-lint-rule.h"
#include "verible/common/text/symbol.h"
#include "verible/common/text/syntax-tree-context.h"
#include "verible/common/text/token-info.h"
#include "verible/verilog/analysis/descriptions.h"

namespace verilog {
namespace analysis {

// PortTypeCheckRule checks if port types are valid for their direction.
// - input ports must be wire type
// - output ports must be wire or reg type
class PortTypeCheckRule : public verible::SyntaxTreeLintRule {
 public:
  using rule_type = verible::SyntaxTreeLintRule;

  static const LintRuleDescriptor &GetDescriptor();

  void HandleSymbol(const verible::Symbol &symbol,
                    const verible::SyntaxTreeContext &context) final;

  verible::LintRuleStatus Report() const final;

 private:
  // Helper function to check port type and report violation if invalid
  void CheckPortType(std::string_view direction,
                     const verible::TokenInfo &type_token,
                     const verible::TokenInfo &identifier_token,
                     const verible::SyntaxTreeContext &context);

  // Check if the given type is valid for the direction
  static bool IsValidTypeForDirection(std::string_view direction,
                                      std::string_view type);

  // Violations
  std::set<verible::LintViolation> violations_;
};

}  // namespace analysis
}  // namespace verilog

#endif  // VERIBLE_VERILOG_ANALYSIS_CHECKERS_PORT_TYPE_CHECK_RULE_H_
