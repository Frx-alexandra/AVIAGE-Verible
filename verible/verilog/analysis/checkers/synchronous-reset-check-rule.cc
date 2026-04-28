// Copyright 2017-2023 The Verible Authors.
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

#include "verible/verilog/analysis/checkers/synchronous-reset-check-rule.h"

#include <cctype>
#include <functional>
#include <set>
#include <string_view>
#include <vector>

#include "verible/common/analysis/lint-rule-status.h"
#include "verible/common/analysis/matcher/bound-symbol-manager.h"
#include "verible/common/analysis/matcher/matcher.h"
#include "verible/common/analysis/syntax-tree-search.h"
#include "verible/common/text/concrete-syntax-leaf.h"
#include "verible/common/text/concrete-syntax-tree.h"
#include "verible/common/text/symbol.h"
#include "verible/common/text/syntax-tree-context.h"
#include "verible/common/text/tree-utils.h"
#include "verible/verilog/CST/statement.h"
#include "verible/verilog/CST/verilog-matchers.h"
#include "verible/verilog/CST/verilog-nonterminals.h"
#include "verible/verilog/analysis/descriptions.h"
#include "verible/verilog/analysis/lint-rule-registry.h"
#include "verible/verilog/parser/verilog-token-enum.h"

namespace verilog {
namespace analysis {

using verible::LintRuleStatus;
using verible::LintViolation;
using verible::SyntaxTreeContext;
using verible::matcher::Matcher;

VERILOG_REGISTER_LINT_RULE(SynchronousResetCheckRule);

static constexpr std::string_view kMessageAsyncReset =
    "Synchronous reset required: sensitivity list should only contain clock "
    "signal, reset should be checked inside the block.";

static constexpr std::string_view kMessageNoReset =
    "Synchronous reset required: block must check a reset signal in the first "
    "if condition.";

static constexpr std::string_view kMessageResetNotFirst =
    "Synchronous reset required: reset signal must be checked in the first if "
    "condition of the block.";

static constexpr std::string_view kMessageNoClock =
    "Synchronous reset required: sensitivity list must contain a clock signal "
    "(containing 'clk' or 'clock').";

const LintRuleDescriptor &SynchronousResetCheckRule::GetDescriptor() {
  static const LintRuleDescriptor d{
      .name = "synchronous-reset-check",
      .topic = "sequential-logic",
      .desc =
          "Checks that always blocks use synchronous reset: sensitivity list "
          "should only contain a clock signal (name containing 'clk' or "
          "'clock'), and the block should check a reset signal (name "
          "containing 'rst' or 'reset') in the first if condition.",
  };
  return d;
}

// Helper function to convert string to lowercase for case-insensitive
// comparison
static std::string ToLower(std::string_view str) {
  std::string result;
  result.reserve(str.size());
  for (char c : str) {
    result.push_back(std::tolower(static_cast<unsigned char>(c)));
  }
  return result;
}

bool SynchronousResetCheckRule::IsClockSignal(std::string_view signal_name) {
  std::string lower = ToLower(signal_name);
  return lower.find("clk") != std::string::npos ||
         lower.find("clock") != std::string::npos;
}

bool SynchronousResetCheckRule::IsResetSignal(std::string_view signal_name) {
  std::string lower = ToLower(signal_name);
  return lower.find("rst") != std::string::npos ||
         lower.find("reset") != std::string::npos;
}

// Extract all signal names from the event control (sensitivity list)
static std::vector<std::string_view> ExtractSensitivitySignals(
    const verible::Symbol &event_control) {
  std::vector<std::string_view> signals;

  // Search for all SymbolIdentifier leaves
  const auto &matches =
      verible::SearchSyntaxTree(event_control, SymbolIdentifierLeaf());

  for (const auto &match : matches) {
    if (const auto *leaf = verible::GetLeftmostLeaf(*match.match)) {
      signals.push_back(leaf->get().text());
    }
  }

  return signals;
}

// Find the first if statement in the always block body
static const verible::SyntaxTreeNode *FindFirstIfStatement(
    const verible::Symbol &always_statement) {
  // Get the always statement node
  const auto &always_node = verible::SymbolCastToNode(always_statement);

  // Get the procedural timing control statement (contains event control)
  const auto *proc_timing_ctrl =
      GetProceduralTimingControlFromAlways(always_node);
  if (!proc_timing_ctrl) {
    return nullptr;
  }

  // Get the statement body (after the event control)
  const auto *statement_body =
      GetProceduralTimingControlStatementBody(*proc_timing_ctrl);
  if (!statement_body) {
    return nullptr;
  }

  // Search for conditional statements (if statements) in the body
  const auto &conditional_matches =
      verible::SearchSyntaxTree(*statement_body, NodekConditionalStatement());

  // Find the first conditional statement that is not nested inside another
  // conditional
  for (const auto &match : conditional_matches) {
    // Check if this conditional is the top-level one by verifying
    // there are no other conditional statements in its context
    bool is_top_level = true;
    for (const auto *context_node : match.context) {
      if (context_node->Kind() == verible::SymbolKind::kNode) {
        const auto &node_ref =
            static_cast<const verible::SyntaxTreeNode &>(*context_node);
        if (node_ref.Tag().tag ==
            static_cast<int>(NodeEnum::kConditionalStatement)) {
          is_top_level = false;
          break;
        }
      }
    }

    if (is_top_level && match.match->Kind() == verible::SymbolKind::kNode) {
      return &verible::SymbolCastToNode(*match.match);
    }
  }

  return nullptr;
}

// Check if an expression contains a reset signal
static bool ExpressionContainsResetSignal(
    const verible::Symbol &expression,
    const std::function<bool(std::string_view)> &is_reset) {
  // Search for all identifiers in the expression
  const auto &matches =
      verible::SearchSyntaxTree(expression, SymbolIdentifierLeaf());

  for (const auto &match : matches) {
    if (const auto *leaf = verible::GetLeftmostLeaf(*match.match)) {
      if (is_reset(leaf->get().text())) {
        return true;
      }
    }
  }

  return false;
}

// Check if the always block has a reset check in the first if condition
bool SynchronousResetCheckRule::HasResetCheck(
    const verible::Symbol &always_statement) {
  // Find the first if statement in the always block
  const auto *first_if = FindFirstIfStatement(always_statement);
  if (!first_if) {
    return false;  // No if statement found
  }

  // Get the if clause from the conditional statement
  const auto *if_clause = GetConditionalStatementIfClause(*first_if);
  if (!if_clause) {
    return false;
  }

  // Get the if header (contains the condition)
  const auto *if_header = GetIfClauseHeader(*if_clause);
  if (!if_header) {
    return false;
  }

  // Get the expression from the if header
  const auto *expression = GetIfHeaderExpression(*if_header);
  if (!expression) {
    return false;
  }

  // Check if the expression contains a reset signal
  return ExpressionContainsResetSignal(
      *expression, [](std::string_view name) -> bool {
        return SynchronousResetCheckRule::IsResetSignal(name);
      });
}

void SynchronousResetCheckRule::HandleSymbol(const verible::Symbol &symbol,
                                             const SyntaxTreeContext &context) {
  // Match legacy 'always' statements (not always_ff, always_comb, always_latch)
  static const Matcher always_matcher(NodekAlwaysStatement(AlwaysKeyword()));

  verible::matcher::BoundSymbolManager manager;
  if (!always_matcher.Matches(symbol, &manager)) {
    return;
  }

  // Get the always statement node
  const auto &always_node = verible::SymbolCastToNode(symbol);

  // Get the procedural timing control statement (contains event control)
  const auto *proc_timing_ctrl =
      GetProceduralTimingControlFromAlways(always_node);
  if (!proc_timing_ctrl) {
    return;
  }

  // Get the event control (sensitivity list)
  const auto *event_control =
      GetEventControlFromProceduralTimingControl(*proc_timing_ctrl);
  if (!event_control) {
    return;
  }

  // Extract all signals from sensitivity list
  auto sensitivity_signals = ExtractSensitivitySignals(*event_control);

  // Get the leftmost leaf (the 'always' keyword) for violation location
  const verible::SyntaxTreeLeaf *always_leaf = verible::GetLeftmostLeaf(symbol);
  if (!always_leaf) {
    return;
  }

  // Check 1: Sensitivity list must have exactly one signal
  // and it must be a clock signal
  if (sensitivity_signals.size() != 1) {
    violations_.insert(
        LintViolation(*always_leaf, kMessageAsyncReset, context));
    return;
  }

  // Check if the single signal is a clock signal
  if (!IsClockSignal(sensitivity_signals[0])) {
    violations_.insert(LintViolation(*always_leaf, kMessageNoClock, context));
    return;
  }

  // Check 2: Block must contain a reset signal check
  if (!HasResetCheck(symbol)) {
    violations_.insert(LintViolation(*always_leaf, kMessageNoReset, context));
    return;
  }
}

LintRuleStatus SynchronousResetCheckRule::Report() const {
  return LintRuleStatus(violations_, GetDescriptor());
}

}  // namespace analysis
}  // namespace verilog
