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

#include <memory>
#include <set>
#include <string_view>

#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "re2/re2.h"
#include "verible/common/analysis/lint-rule-status.h"
#include "verible/common/analysis/text-structure-lint-rule.h"
#include "verible/common/text/text-structure.h"
#include "verible/verilog/analysis/descriptions.h"

namespace verilog {
namespace analysis {

static constexpr std::string_view kDefaultBorderRegex = "^//={58}$";
static constexpr std::string_view kDefaultCommentRegex = "^//\\s*.*$";

// Detects whether multi-line comments are surrounded by uniform format.
class MultiLineCommentsRule : public verible::TextStructureLintRule {
 public:
  using rule_type = verible::TextStructureLintRule;

  static const LintRuleDescriptor &GetDescriptor();

  MultiLineCommentsRule()
      : border_regex_(
            std::make_unique<re2::RE2>(kDefaultBorderRegex, re2::RE2::Quiet)),
        comment_regex_(std::make_unique<re2::RE2>(kDefaultCommentRegex,
                                                  re2::RE2::Quiet)) {}

  absl::Status Configure(std::string_view configuration) final;

  void Lint(const verible::TextStructureView &, std::string_view) final;

  verible::LintRuleStatus Report() const final;

 private:
  // Collection of found violations.
  std::set<verible::LintViolation> violations_;

  // Configurable regex patterns for border and comment format
  std::unique_ptr<re2::RE2> border_regex_;
  std::unique_ptr<re2::RE2> comment_regex_;

  // Default comment border format message
  std::string format_message_ =
      "two forward slashes followed by 58 equal signs";

  std::string CreateViolationMessage() {
    return absl::StrCat(
        "Multi-line comments must be surrounded by uniform format: ",
        format_message_, ".");
  }
};

}  // namespace analysis
}  // namespace verilog

#endif  // VERIBLE_VERILOG_ANALYSIS_CHECKERS_MULTI_LINE_COMMENTS_RULE_H_