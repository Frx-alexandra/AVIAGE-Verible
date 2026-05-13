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

#ifndef VERIBLE_VERILOG_ANALYSIS_CHECKERS_CONDITIONAL_OPERATOR_SPACING_RULE_H_
#define VERIBLE_VERILOG_ANALYSIS_CHECKERS_CONDITIONAL_OPERATOR_SPACING_RULE_H_

#include <set>
#include <string_view>

#include "verible/common/analysis/lint-rule-status.h"
#include "verible/common/analysis/text-structure-lint-rule.h"
#include "verible/common/text/text-structure.h"
#include "verible/verilog/analysis/descriptions.h"

namespace verilog {
namespace analysis {

// ConditionalOperatorSpacingRule checks that conditional/comparison operators
// have equal spacing on both sides on each line.
//
// This rule ensures consistent formatting around operators like:
// ==, !=, <, >, <=, >=, ===, !==
//
// Examples:
//   Good: if (a == b)
//   Good: if (a==b)
//   Bad:  if (a ==b)   // Different spacing
//   Bad:  if (a== b)   // Different spacing
//
class ConditionalOperatorSpacingRule : public verible::TextStructureLintRule {
 public:
  using rule_type = verible::TextStructureLintRule;

  static const LintRuleDescriptor &GetDescriptor();

  ConditionalOperatorSpacingRule() = default;

  void Lint(const verible::TextStructureView &text_structure,
            std::string_view filename) final;

  verible::LintRuleStatus Report() const final;

 private:
  // Collection of found violations.
  std::set<verible::LintViolation> violations_;
};

}  // namespace analysis
}  // namespace verilog

#endif  // VERIBLE_VERILOG_ANALYSIS_CHECKERS_CONDITIONAL_OPERATOR_SPACING_RULE_H_
