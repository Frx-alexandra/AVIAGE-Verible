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

#ifndef VERIBLE_VERILOG_ANALYSIS_CHECKERS_MULTI_LINE_COMMENTS_RULE_H_
#define VERIBLE_VERILOG_ANALYSIS_CHECKERS_MULTI_LINE_COMMENTS_RULE_H_

#include <set>
#include <string_view>

#include "absl/status/status.h"
#include "verible/common/analysis/lint-rule-status.h"
#include "verible/common/analysis/text-structure-lint-rule.h"
#include "verible/common/text/text-structure.h"
#include "verible/verilog/analysis/descriptions.h"

namespace verilog {
namespace analysis {

// Detects whether multi-line comments are surrounded by uniform format.
class MultiLineCommentsRule : public verible::TextStructureLintRule {
 public:
  using rule_type = verible::TextStructureLintRule;

  static const LintRuleDescriptor &GetDescriptor();

  MultiLineCommentsRule() = default;

  absl::Status Configure(std::string_view configuration) final;

  void Lint(const verible::TextStructureView &, std::string_view) final;

  verible::LintRuleStatus Report() const final;

 private:
  // Collection of found violations.
  std::set<verible::LintViolation> violations_;

  // Configurable border pattern (default:
  // "===========================================================")
  std::string border_pattern_ = std::string(59, '=');

  // Default comment border format message
  std::string format_message_ =
      "two forward slashes followed by 59 equal signs";
};

}  // namespace analysis
}  // namespace verilog

#endif  // VERIBLE_VERILOG_ANALYSIS_CHECKERS_MULTI_LINE_COMMENTS_RULE_H_