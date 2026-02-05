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

#include "verible/verilog/analysis/checkers/one-statement-per-line-rule.h"

#include <set>
#include <string_view>
#include <vector>

#include "verible/common/analysis/lint-rule-status.h"
#include "verible/common/analysis/syntax-tree-search.h"
#include "verible/common/text/text-structure.h"
#include "verible/common/text/token-info.h"
#include "verible/common/text/tree-utils.h"
#include "verible/verilog/CST/statement.h"
#include "verible/verilog/CST/verilog-matchers.h"
#include "verible/verilog/CST/verilog-nonterminals.h"
#include "verible/verilog/analysis/descriptions.h"
#include "verible/verilog/analysis/lint-rule-registry.h"

namespace verilog {
namespace analysis {

using verible::LintRuleStatus;
using verible::LintViolation;
using verible::SyntaxTreeNode;
using verible::TextStructureView;
using verible::TokenInfo;

// Register OneStatementPerLineRule.
VERILOG_REGISTER_LINT_RULE(OneStatementPerLineRule);

static constexpr std::string_view kMessage =
    "each statement should begin on a new line";

const LintRuleDescriptor &OneStatementPerLineRule::GetDescriptor() {
  static const LintRuleDescriptor d{
      .name = "one-statement-per-line",
      .topic = "statement-formatting",
      .desc =
          "Check that each statement begins on a new line. "
          "Multiple statements on the same line should be split into "
          "separate lines.",
  };
  return d;
}

// Get the first token of a statement (where it starts)
static const TokenInfo *GetStatementStartToken(const verible::Symbol &symbol) {
  if (symbol.Kind() == verible::SymbolKind::kLeaf) {
    return &verible::SymbolCastToLeaf(symbol).get();
  }
  // For nodes, recursively find the first leaf
  const auto &syntax_node = verible::SymbolCastToNode(symbol);
  for (size_t i = 0; i < syntax_node.size(); ++i) {
    const auto &child = syntax_node[i];
    if (child == nullptr) continue;
    const auto *token = GetStatementStartToken(*child);
    if (token != nullptr) return token;
  }
  return nullptr;
}

void OneStatementPerLineRule::Lint(const TextStructureView &text_structure,
                                   std::string_view filename) {
  const auto *root = text_structure.SyntaxTree().get();
  if (!root) return;

  // Track which lines have statement starts
  std::set<int> seen_lines;

  // Find all statements - check various statement types
  // kNetVariableAssignment covers assignments like "a = 1;"
  // This includes both procedural and continuous assignments
  const auto net_var_assigns =
      verible::SearchSyntaxTree(*root, NodekNetVariableAssignment());
  const auto statements = verible::SearchSyntaxTree(*root, NodekStatement());

  // Process all statement types
  auto process_statements = [&](const auto &matches) {
    for (const auto &match : matches) {
      const auto *start_token = GetStatementStartToken(*match.match);
      if (!start_token) continue;

      const auto line_column =
          text_structure.GetRangeForToken(*start_token).start;
      int line_number = line_column.line;

      // Check if we've already seen a statement start on this line
      if (seen_lines.count(line_number) > 0) {
        violations_.insert(LintViolation(*start_token, kMessage));
      } else {
        seen_lines.insert(line_number);
      }
    }
  };

  process_statements(net_var_assigns);
  process_statements(statements);
}

LintRuleStatus OneStatementPerLineRule::Report() const {
  return LintRuleStatus(violations_, GetDescriptor());
}

}  // namespace analysis
}  // namespace verilog
