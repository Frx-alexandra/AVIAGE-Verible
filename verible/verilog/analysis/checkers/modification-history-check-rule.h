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

#ifndef VERIBLE_VERILOG_ANALYSIS_CHECKERS_MODIFICATION_HISTORY_CHECK_RULE_H_
#define VERIBLE_VERILOG_ANALYSIS_CHECKERS_MODIFICATION_HISTORY_CHECK_RULE_H_

#include <set>
#include <string>
#include <string_view>

#include "absl/status/status.h"
#include "verible/common/analysis/lint-rule-status.h"
#include "verible/common/analysis/text-structure-lint-rule.h"
#include "verible/common/text/text-structure.h"
#include "verible/verilog/analysis/descriptions.h"

namespace verilog {
namespace analysis {

// ModificationHistoryCheckRule checks that every file contains the text
// "MODIFICATION HISTORY" in the comments before any module definition.
class ModificationHistoryCheckRule : public verible::TextStructureLintRule {
 public:
  using rule_type = verible::TextStructureLintRule;

  ModificationHistoryCheckRule() = default;

  static const LintRuleDescriptor &GetDescriptor();

  void Lint(const verible::TextStructureView &text_structure,
            std::string_view text) final;

  verible::LintRuleStatus Report() const final;

  absl::Status Configure(std::string_view configuration) final;

 private:
  std::set<verible::LintViolation> violations_;
};

}  // namespace analysis
}  // namespace verilog

#endif  // VERIBLE_VERILOG_ANALYSIS_CHECKERS_MODIFICATION_HISTORY_CHECK_RULE_H_
