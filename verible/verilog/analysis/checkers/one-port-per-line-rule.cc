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

#include "verible/verilog/analysis/checkers/one-port-per-line-rule.h"

#include <set>
#include <string_view>

#include "verible/common/analysis/lint-rule-status.h"
#include "verible/common/analysis/syntax-tree-search.h"
#include "verible/common/text/text-structure.h"
#include "verible/common/text/token-info.h"
#include "verible/verilog/CST/identifier.h"
#include "verible/verilog/CST/port.h"
#include "verible/verilog/CST/verilog-matchers.h"
#include "verible/verilog/analysis/descriptions.h"
#include "verible/verilog/analysis/lint-rule-registry.h"

namespace verilog {
namespace analysis {

using verible::LintRuleStatus;
using verible::LintViolation;
using verible::TextStructureView;
using verible::TokenInfo;

// Register OnePortPerLineRule.
VERILOG_REGISTER_LINT_RULE(OnePortPerLineRule);

static constexpr std::string_view kMessage =
    "only one port declaration is allowed per line";

const LintRuleDescriptor &OnePortPerLineRule::GetDescriptor() {
  static const LintRuleDescriptor d{
      .name = "one-port-per-line",
      .topic = "port-declarations",
      .desc =
          "Check that only one port is declared per line. "
          "Multiple ports on the same line should be split into separate "
          "lines.",
  };
  return d;
}

void OnePortPerLineRule::Lint(const TextStructureView &text_structure,
                              std::string_view filename) {
  // Find all port declarations in the syntax tree
  const auto *root = text_structure.SyntaxTree().get();
  if (!root) return;

  // Track which lines have port declarations
  std::set<int> seen_lines;

  // Find all port declarations (e.g., "input wire a")
  const auto port_declarations = FindAllPortDeclarations(*root);
  for (const auto &port_match : port_declarations) {
    const auto *identifier_leaf =
        GetIdentifierFromPortDeclaration(*port_match.match);
    if (!identifier_leaf) continue;

    const auto &token = identifier_leaf->get();
    const auto line_column = text_structure.GetRangeForToken(token).start;
    int line_number = line_column.line;

    if (seen_lines.count(line_number) > 0) {
      violations_.insert(LintViolation(token, kMessage));
    } else {
      seen_lines.insert(line_number);
    }
  }

  // Find all ports (e.g., the "b" in "input a, b")
  // These are kPort nodes inside kPortDeclarationList
  const auto ports = verible::SearchSyntaxTree(*root, NodekPort());
  for (const auto &port : ports) {
    // Get the identifier from the port
    const auto *port_ref = GetPortReferenceFromPort(*port.match);
    if (!port_ref) continue;
    const auto *identifier_leaf = GetIdentifierFromPortReference(*port_ref);
    if (!identifier_leaf) continue;

    const auto &token = identifier_leaf->get();
    const auto line_column = text_structure.GetRangeForToken(token).start;
    int line_number = line_column.line;

    if (seen_lines.count(line_number) > 0) {
      violations_.insert(LintViolation(token, kMessage));
    } else {
      seen_lines.insert(line_number);
    }
  }
}

LintRuleStatus OnePortPerLineRule::Report() const {
  return LintRuleStatus(violations_, GetDescriptor());
}

}  // namespace analysis
}  // namespace verilog
