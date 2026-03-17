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

#ifndef VERIBLE_VERILOG_ANALYSIS_CHECKERS_FORBIDDEN_NAME_CONSTANT_WITH_VALUE_RULE_H_
#define VERIBLE_VERILOG_ANALYSIS_CHECKERS_FORBIDDEN_NAME_CONSTANT_WITH_VALUE_RULE_H_

#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "absl/status/status.h"
#include "verible/common/analysis/lint-rule-status.h"
#include "verible/common/analysis/token-stream-lint-rule.h"
#include "verible/common/text/token-info.h"
#include "verible/verilog/analysis/descriptions.h"

namespace verilog {
namespace analysis {

// ForbiddenNameConstantWithValueRule checks that when defining a constant
// using a macro definition (`define), the macro name does not contain the
// numerical value being defined. This prevents names like `define C_1001
// 4'b1001 and encourages descriptive names like `define C_CTRL_ADDR 4'b1001.
// Supports binary (e.g., 4'b1001), decimal (e.g., 1001), and hexadecimal
// (e.g., 8'hFF) literals.
class ForbiddenNameConstantWithValueRule : public verible::TokenStreamLintRule {
 public:
  using rule_type = verible::TokenStreamLintRule;

  ForbiddenNameConstantWithValueRule();

  static const LintRuleDescriptor &GetDescriptor();

  void HandleToken(const verible::TokenInfo &token) final;

  verible::LintRuleStatus Report() const final;

  absl::Status Configure(std::string_view configuration) final;

 private:
  // States of the internal token-based analysis.
  enum class State {
    kNormal,
    kExpectPPIdentifier,
    kExpectMacroValue,
  };

  // Internal lexical analysis state.
  State state_ = State::kNormal;

  // Store the macro name temporarily for checking
  std::string current_macro_name_;
  verible::TokenInfo current_macro_token_;

  // Track the base type for based literals (e.g., 'b, 'h, 'o)
  int base_for_literal_ = 10;  // Default to decimal

  std::set<verible::LintViolation> violations_;

  // Extract numeric value from token text
  // If base_hint is provided, it overrides auto-detection
  // Returns true if a numeric value was extracted
  bool ExtractNumericValue(std::string_view text, int64_t *value,
                           int base_hint = 0);

  // Get all possible string representations of a number (decimal, hex, binary)
  std::vector<std::string> GetNumberRepresentations(int64_t value);

  // Check if macro name contains the value in specific bases
  // bases: bitwise OR of which bases to check (1=binary, 8=octal, 10=decimal,
  // 16=hex)
  bool NameContainsValue(const std::string &name, int64_t value,
                         int bases_to_check = 0xFF);
};

}  // namespace analysis
}  // namespace verilog

#endif  // VERIBLE_VERILOG_ANALYSIS_CHECKERS_FORBIDDEN_NAME_CONSTANT_WITH_VALUE_RULE_H_
