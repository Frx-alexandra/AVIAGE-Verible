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

#ifndef VERIBLE_VERILOG_ANALYSIS_CHECKERS_WIRE_REG_DECLARATION_ALIGNMENT_RULE_H_
#define VERIBLE_VERILOG_ANALYSIS_CHECKERS_WIRE_REG_DECLARATION_ALIGNMENT_RULE_H_

#include <set>
#include <string_view>
#include <vector>

#include "verible/common/analysis/lint-rule-status.h"
#include "verible/common/analysis/text-structure-lint-rule.h"
#include "verible/common/text/text-structure.h"
#include "verible/verilog/analysis/descriptions.h"

namespace verilog {
namespace analysis {

// WireRegDeclarationAlignmentRule checks that wire and reg declarations
// within a file follow consistent alignment for:
// - Start of length definition (dimension/range)
// - Start of signal name
// - Position of trailing semicolon
//
// The rule follows the first declaration style in the file and reports
// violations for inconsistent declarations.
class WireRegDeclarationAlignmentRule : public verible::TextStructureLintRule {
 public:
  using rule_type = verible::TextStructureLintRule;

  static const LintRuleDescriptor &GetDescriptor();

  void Lint(const verible::TextStructureView &, std::string_view) final;

  verible::LintRuleStatus Report() const final;

 private:
  // Structure to hold information about a declaration
  struct DeclarationInfo {
    size_t line_number;
    size_t keyword_end;  // Position after "wire" or "reg"
    bool has_range;      // Whether declaration has a range like [7:0]
    size_t range_start;  // Position of '[' (only valid if has_range is true)
    size_t name_start;   // Position of signal name
    size_t semicolon_pos;  // Position of ';'
    std::string_view line_text;
  };

  // Collection of found violations
  std::set<verible::LintViolation> violations_;

  // Expected alignment positions
  struct AlignmentStyle {
    bool initialized = false;
    size_t first_decl_line = 0;       // Line number of first declaration (for name/semicolon reference)
    size_t first_range_line = 0;      // Line number of first declaration with range (for range reference)
    bool has_range_style = false;     // Whether we've seen any range yet
    size_t keyword_end = 0;
    size_t range_start = 0;           // Column where ranges should start (set when first range seen)
    size_t name_start = 0;            // Column where names should start (from first declaration)
    size_t semicolon_pos = 0;         // Column where semicolons should be (from first declaration)
  };

  AlignmentStyle expected_style_;

  // Parse a line to extract declaration information
  bool ParseDeclaration(std::string_view line, size_t line_number,
                        DeclarationInfo *info);

  // Check if alignment matches expected style
  void CheckAlignment(const DeclarationInfo &info);
};

}  // namespace analysis
}  // namespace verilog

#endif  // VERIBLE_VERILOG_ANALYSIS_CHECKERS_WIRE_REG_DECLARATION_ALIGNMENT_RULE_H_
