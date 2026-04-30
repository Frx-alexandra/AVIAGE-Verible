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

#include "verible/verilog/analysis/checkers/signal-initialization-rule.h"

#include <cctype>
#include <set>
#include <string_view>

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

VERILOG_REGISTER_LINT_RULE(SignalInitializationRule);

static constexpr std::string_view kMessage =
    "Signal '%s' is assigned in the always block but not initialized in the "
    "reset branch. All signals with non-blocking assignments must be "
    "initialized.";

const LintRuleDescriptor &SignalInitializationRule::GetDescriptor() {
  static const LintRuleDescriptor d{
      .name = "signal-initialization",
      .topic = "sequential-logic",
      .desc =
          "Checks that all signals assigned with non-blocking assignments in "
          "always blocks are properly initialized in the reset branch. The "
          "rule applies to always blocks where the first if condition checks "
          "a reset signal (name containing 'rst' or 'reset'). All signals on "
          "the left-hand side of non-blocking assignments must be initialized "
          "to a constant or macro in the reset branch.",
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

// Helper function to check if a signal name contains reset keywords
static bool IsResetSignal(std::string_view signal_name) {
  std::string lower = ToLower(signal_name);
  return lower.find("rst") != std::string::npos ||
         lower.find("reset") != std::string::npos;
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
static bool ExpressionContainsResetSignal(const verible::Symbol &expression) {
  // Search for all identifiers in the expression
  const auto &matches =
      verible::SearchSyntaxTree(expression, SymbolIdentifierLeaf());

  for (const auto &match : matches) {
    if (const auto *leaf = verible::GetLeftmostLeaf(*match.match)) {
      if (IsResetSignal(leaf->get().text())) {
        return true;
      }
    }
  }

  return false;
}

// Check if the first if statement in the always block checks a reset signal
bool SignalInitializationRule::HasResetCheckInFirstIf(
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
  return ExpressionContainsResetSignal(*expression);
}

// Extract the signal name from the left-hand side of an assignment
static std::string_view ExtractSignalNameFromLhs(
    const verible::Symbol &lhs_symbol) {
  // Search for the identifier in the LHS
  const auto &matches =
      verible::SearchSyntaxTree(lhs_symbol, SymbolIdentifierLeaf());

  if (!matches.empty()) {
    if (const auto *leaf = verible::GetLeftmostLeaf(*matches[0].match)) {
      return leaf->get().text();
    }
  }

  return "";
}

// Get all signals assigned with non-blocking assignments in the always block
std::set<std::string_view> SignalInitializationRule::GetNonBlockingAssignedSignals(
    const verible::Symbol &always_statement) {
  std::set<std::string_view> signals;

  // Find all non-blocking assignments in the always block
  const auto &assignments =
      FindAllNonBlockingAssignments(always_statement);

  for (const auto &assignment : assignments) {
    if (assignment.match->Kind() == verible::SymbolKind::kNode) {
      const auto &assignment_node =
          verible::SymbolCastToNode(*assignment.match);

      // Get the LHS of the assignment
      const auto *lhs = GetNonBlockingAssignmentLhs(assignment_node);
      if (lhs) {
        std::string_view signal_name = ExtractSignalNameFromLhs(*lhs);
        if (!signal_name.empty()) {
          signals.insert(signal_name);
        }
      }
    }
  }

  return signals;
}

// Check if the RHS contains only constants, parameters, and macros
// Does NOT accept signal references or other variable identifiers
bool SignalInitializationRule::IsConstantOrMacroAssignment(
    const verible::Symbol &assignment_rhs,
    std::string_view lhs_signal_name) {
  // Get all leaf tokens in the RHS expression
  std::vector<const verible::SyntaxTreeLeaf*> leaves;
  const std::function<void(const verible::Symbol&)> collect_leaves = 
      [&](const verible::Symbol& symbol) {
    if (symbol.Kind() == verible::SymbolKind::kLeaf) {
      leaves.push_back(&verible::SymbolCastToLeaf(symbol));
    } else {
      const auto& node = verible::SymbolCastToNode(symbol);
      for (const auto& child : node.children()) {
        if (child) collect_leaves(*child);
      }
    }
  };
  collect_leaves(assignment_rhs);

  if (leaves.empty()) {
    return false;
  }

  // Check each leaf token
  for (const auto* leaf : leaves) {
    const auto token_enum = static_cast<verilog_tokentype>(leaf->get().token_enum());
    std::string_view text = leaf->get().text();
    
    // Accept: Number literals (constants)
    if (token_enum == TK_DecNumber || token_enum == TK_UnBasedNumber) {
      continue;
    }
    
    // Accept: Macros (start with `)
    if (!text.empty() && text[0] == '`') {
      continue;
    }
    
    // Accept: Operators (+, -, *, /, <<, >>, etc.)
    // These are typically punctuation or special tokens, not identifiers
    if (token_enum != SymbolIdentifier) {
      // Allow operators, parentheses, brackets, etc.
      continue;
    }
    
    // At this point, we have an identifier
    // REJECT: If it's the signal being assigned (self-reference)
    if (text == lhs_signal_name) {
      return false;
    }
    
    // For other identifiers, we need to determine if they are parameters/constants
    // In SystemVerilog, we cannot easily distinguish at parse time between:
    // - Parameters/localparams (should accept)
    // - Signal variables (should reject)
    // 
    // Conservative approach: ACCEPT identifiers that follow parameter naming conventions
    // Common conventions:
    // - ALL_CAPS (e.g., RESET_VAL, INIT_STATE)
    // - Start with uppercase (e.g., ResetValue, InitState)
    // 
    // REJECT identifiers that follow signal naming conventions:
    // - Start with lowercase (e.g., counter, state, data)
    // - Start with r_, w_, i_, o_ prefixes (common signal prefixes)
    
    if (text.empty()) {
      return false;
    }
    
    // Check if identifier starts with signal-like prefix
    if (text.size() >= 2 && text[1] == '_') {
      char prefix = text[0];
      if (prefix == 'r' || prefix == 'w' || prefix == 'i' || prefix == 'o') {
        // Looks like a signal with prefix (r_, w_, i_, o_)
        return false;
      }
    }
    
    // Check if identifier is all uppercase (likely a parameter/constant)
    bool all_upper = true;
    bool has_lower = false;
    for (char c : text) {
      if (std::isalpha(static_cast<unsigned char>(c))) {
        if (std::islower(static_cast<unsigned char>(c))) {
          has_lower = true;
          all_upper = false;
          break;
        }
      }
    }
    
    // If all uppercase (or has underscores/digits), accept as parameter
    if (all_upper) {
      continue;
    }
    
    // If starts with lowercase, likely a signal/variable - reject
    if (has_lower && std::islower(static_cast<unsigned char>(text[0]))) {
      return false;
    }
    
    // If starts with uppercase (CamelCase), might be parameter - accept
    if (std::isupper(static_cast<unsigned char>(text[0]))) {
      continue;
    }
    
    // Default: reject unknown identifiers to be conservative
    return false;
  }

  // All tokens checked out - this is a valid constant/parameter expression
  return true;
}

// Get all signals initialized in the reset branch
std::set<std::string_view> SignalInitializationRule::GetInitializedSignalsInResetBranch(
    const verible::Symbol &always_statement) {
  std::set<std::string_view> initialized_signals;

  // Find the first if statement
  const auto *first_if = FindFirstIfStatement(always_statement);
  if (!first_if) {
    return initialized_signals;
  }

  // Get the if clause (the reset branch)
  const auto *if_clause = GetConditionalStatementIfClause(*first_if);
  if (!if_clause) {
    return initialized_signals;
  }

  // Get the statement body of the if clause
  const auto *if_body = GetIfClauseStatementBody(*if_clause);
  if (!if_body) {
    return initialized_signals;
  }

  // Find all non-blocking assignments in the reset branch
  const auto &assignments = FindAllNonBlockingAssignments(*if_body);

  for (const auto &assignment : assignments) {
    if (assignment.match->Kind() == verible::SymbolKind::kNode) {
      const auto &assignment_node =
          verible::SymbolCastToNode(*assignment.match);

      // Get the LHS and RHS of the assignment
      const auto *lhs = GetNonBlockingAssignmentLhs(assignment_node);
      const auto *rhs = GetNonBlockingAssignmentRhs(assignment_node);

      if (lhs && rhs) {
        std::string_view signal_name = ExtractSignalNameFromLhs(*lhs);
        // Check if RHS is a constant or macro (and doesn't reference the signal itself)
        if (!signal_name.empty() && IsConstantOrMacroAssignment(*rhs, signal_name)) {
          initialized_signals.insert(signal_name);
        }
      }
    }
  }

  return initialized_signals;
}

void SignalInitializationRule::HandleSymbol(const verible::Symbol &symbol,
                                            const SyntaxTreeContext &context) {
  // Match all types of always statements (always, always_ff, always_comb, etc.)
  static const Matcher always_matcher(NodekAlwaysStatement());

  verible::matcher::BoundSymbolManager manager;
  if (!always_matcher.Matches(symbol, &manager)) {
    return;
  }

  // Check if this always block has a reset check in the first if condition
  if (!HasResetCheckInFirstIf(symbol)) {
    return;  // Not applicable to this always block
  }

  // Get all signals assigned with non-blocking assignments
  auto assigned_signals = GetNonBlockingAssignedSignals(symbol);

  // Get all signals initialized in the reset branch
  auto initialized_signals = GetInitializedSignalsInResetBranch(symbol);

  // Find signals that are assigned but not initialized
  for (const auto &signal : assigned_signals) {
    if (initialized_signals.find(signal) == initialized_signals.end()) {
      // Signal is not initialized in reset branch - violation
      // Find the first occurrence of this signal in a non-blocking assignment
      // to report the violation location
      const auto &assignments = FindAllNonBlockingAssignments(symbol);
      
      for (const auto &assignment : assignments) {
        if (assignment.match->Kind() == verible::SymbolKind::kNode) {
          const auto &assignment_node =
              verible::SymbolCastToNode(*assignment.match);
          const auto *lhs = GetNonBlockingAssignmentLhs(assignment_node);
          
          if (lhs) {
            std::string_view lhs_signal = ExtractSignalNameFromLhs(*lhs);
            if (lhs_signal == signal) {
              // Found the first occurrence - report violation here
              if (const auto *leaf = verible::GetLeftmostLeaf(*lhs)) {
                // Format the message with the signal name
                std::string formatted_message = 
                    "Signal '" + std::string(signal) + 
                    "' is assigned in the always block but not initialized in the "
                    "reset branch. All signals with non-blocking assignments must be "
                    "initialized in the first if condition that checks the reset signal.";
                violations_.insert(
                    LintViolation(*leaf, formatted_message, context));
              }
              break;  // Only report once per signal
            }
          }
        }
      }
    }
  }
}

LintRuleStatus SignalInitializationRule::Report() const {
  return LintRuleStatus(violations_, GetDescriptor());
}

}  // namespace analysis
}  // namespace verilog
