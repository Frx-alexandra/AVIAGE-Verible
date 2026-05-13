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

#ifndef VERIBLE_VERILOG_ANALYSIS_CHECKERS_IP_INSTANTIATION_FORMAT_RULE_H_
#define VERIBLE_VERILOG_ANALYSIS_CHECKERS_IP_INSTANTIATION_FORMAT_RULE_H_

#include <set>
#include <string_view>
#include <vector>

#include "verible/common/analysis/lint-rule-status.h"
#include "verible/common/analysis/text-structure-lint-rule.h"
#include "verible/common/text/text-structure.h"
#include "verible/verilog/analysis/descriptions.h"

namespace verilog {
namespace analysis {

// IPInstantiationFormatRule checks that all IP/module instantiations within a file
// follow consistent formatting for port connections:
// - Position of opening parenthesis '(' for each port connection
// - Position of closing parenthesis ')' for each port connection
//
// The format is established by the first port connection line in the first instance
// found in the file, and all subsequent port connections must match.
class IPInstantiationFormatRule : public verible::TextStructureLintRule {
 public:
  using rule_type = verible::TextStructureLintRule;

  static const LintRuleDescriptor &GetDescriptor();

  void Lint(const verible::TextStructureView &, std::string_view) final;

  verible::LintRuleStatus Report() const final;

 private:
  // Structure to hold information about a port connection line
  struct PortConnectionInfo {
    size_t line_number;
    size_t left_paren_pos;   // Position of '('
    size_t right_paren_pos;  // Position of ')'
    std::string_view line_text;
  };

  // Collection of found violations
  std::set<verible::LintViolation> violations_;

  // Expected format (based on first port connection line in first instance)
  struct FormatStyle {
    bool initialized = false;
    size_t first_connection_line = 0;  // Line number of first port connection
    size_t left_paren_pos = 0;         // Expected position of '('
    size_t right_paren_pos = 0;        // Expected position of ')'
  };

  FormatStyle expected_format_;

  // Parse a line to check if it's a port connection and extract format info
  bool ParsePortConnection(std::string_view line, size_t line_number,
                          PortConnectionInfo *info);

  // Check if format matches expected style
  void CheckFormat(const PortConnectionInfo &info);

  // Check if line is inside an instantiation (has .port_name(...) pattern)
  bool IsPortConnectionLine(std::string_view line);
};

}  // namespace analysis
}  // namespace verilog

#endif  // VERIBLE_VERILOG_ANALYSIS_CHECKERS_IP_INSTANTIATION_FORMAT_RULE_H_
