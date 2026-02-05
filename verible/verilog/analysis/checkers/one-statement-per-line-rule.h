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
#include "verible/common/analysis/text-structure-lint-rule.h"
#include "verible/common/text/text-structure.h"
#include "verible/verilog/analysis/descriptions.h"

namespace verilog {
namespace analysis {

// OneStatementPerLineRule checks that each statement begins on a new line.
// Multiple statements on the same line should be split into separate lines.
class OneStatementPerLineRule : public verible::TextStructureLintRule {
 public:
  using rule_type = verible::TextStructureLintRule;

  static const LintRuleDescriptor &GetDescriptor();

  void Lint(const verible::TextStructureView &text_structure,
            std::string_view filename) final;

  verible::LintRuleStatus Report() const final;

 private:
  // Violations
  std::set<verible::LintViolation> violations_;
};

}  // namespace analysis
}  // namespace verilog

#endif  // VERIBLE_VERILOG_ANALYSIS_CHECKERS_ONE_STATEMENT_PER_LINE_RULE_H_
