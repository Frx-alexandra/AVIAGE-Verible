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

#include "verible/verilog/analysis/checkers/signal-name-prefix-rule.h"

#include <map>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "absl/strings/str_split.h"
#include "verible/common/analysis/lint-rule-status.h"
#include "verible/common/analysis/matcher/bound-symbol-manager.h"
#include "verible/common/analysis/matcher/matcher.h"
#include "verible/common/text/symbol.h"
#include "verible/common/text/syntax-tree-context.h"
#include "verible/common/text/token-info.h"
#include "verible/common/util/logging.h"
#include "verible/verilog/CST/declaration.h"
#include "verible/verilog/CST/identifier.h"
#include "verible/verilog/CST/net.h"
#include "verible/verilog/CST/port.h"
#include "verible/verilog/CST/verilog-matchers.h"
#include "verible/verilog/analysis/descriptions.h"
#include "verible/verilog/analysis/lint-rule-registry.h"

namespace verilog {
namespace analysis {

using verible::LintRuleStatus;
using verible::LintViolation;
using verible::Symbol;
using verible::SyntaxTreeContext;
using verible::TokenInfo;
using verible::matcher::Matcher;

// Register SignalNamePrefixRule.
VERILOG_REGISTER_LINT_RULE(SignalNamePrefixRule);

static constexpr std::string_view kMessageInput =
    "input signal names must start with i_";
static constexpr std::string_view kMessageOutput =
    "output signal names must start with o_";
static constexpr std::string_view kMessageInOut =
    "inout signal names must start with io_";
static constexpr std::string_view kMessageReg =
    "reg signal names must start with r_";
static constexpr std::string_view kMessageWire =
    "wire signal names must start with w_";

const LintRuleDescriptor &SignalNamePrefixRule::GetDescriptor() {
  static const LintRuleDescriptor d{
      .name = "signal-name-prefix",
      .topic = "prefixes-for-signals-and-types",
      .desc =
          "Check that signal names start with i_ for input, o_ for output, "
          "io_ for inout, r_ for reg, and w_ for wire.",
  };
  return d;
}

static const Matcher &PortMatcher() {
  static const Matcher matcher(NodekPortDeclaration());
  return matcher;
}

static const Matcher &NetDeclarationMatcher() {
  static const Matcher matcher(NodekNetDeclaration());
  return matcher;
}

static const Matcher &ModulePortMatcher() {
  static const Matcher matcher(NodekModulePortDeclaration());
  return matcher;
}

void SignalNamePrefixRule::Violation(std::string_view signal_type,
                                     const TokenInfo &token,
                                     const SyntaxTreeContext &context) {
  if (signal_type == "input") {
    violations_.insert(LintViolation(token, kMessageInput, context));
  } else if (signal_type == "output") {
    violations_.insert(LintViolation(token, kMessageOutput, context));
  } else if (signal_type == "inout") {
    violations_.insert(LintViolation(token, kMessageInOut, context));
  } else if (signal_type == "reg") {
    violations_.insert(LintViolation(token, kMessageReg, context));
  } else if (signal_type == "wire") {
    violations_.insert(LintViolation(token, kMessageWire, context));
  }
}

bool SignalNamePrefixRule::IsPrefixCorrect(std::string_view prefix,
                                           std::string_view signal_type) {
  static const std::map<std::string_view, std::set<std::string_view>> prefixes =
      {{"input", {"i_"}},
       {"output", {"o_"}},
       {"inout", {"io_"}},
       {"reg", {"r_"}},
       {"wire", {"w_"}}};

  return prefixes.at(signal_type).count(prefix) == 1;
}

void SignalNamePrefixRule::HandleSymbol(const Symbol &symbol,
                                        const SyntaxTreeContext &context) {
  verible::matcher::BoundSymbolManager manager;

  // Handle port declarations
  if (PortMatcher().Matches(symbol, &manager)) {
    const auto *identifier_leaf = GetIdentifierFromPortDeclaration(symbol);
    const auto *direction_leaf = GetDirectionFromPortDeclaration(symbol);
    const auto token = identifier_leaf->get();
    const auto direction =
        direction_leaf ? direction_leaf->get().text() : "input";
    const auto name = ABSL_DIE_IF_NULL(identifier_leaf)->get().text();

    // Check if prefix is correct
    bool has_correct_prefix = false;
    bool has_expected_prefix = false;

    if (name.size() > 2 && name.substr(0, 2) == "i_") {
      has_expected_prefix = true;
      has_correct_prefix = IsPrefixCorrect("i_", direction);
    } else if (name.size() > 2 && name.substr(0, 2) == "o_") {
      has_expected_prefix = true;
      has_correct_prefix = IsPrefixCorrect("o_", direction);
    } else if (name.size() > 3 && name.substr(0, 3) == "io_") {
      has_expected_prefix = true;
      has_correct_prefix = IsPrefixCorrect("io_", direction);
    }

    // If no expected prefix found, or if prefix is incorrect, it's a violation
    if (!has_expected_prefix || !has_correct_prefix) {
      Violation(direction, token, context);
    }
    return;
  }

  // Handle net declarations (wire, tri, wor, etc.)
  if (NetDeclarationMatcher().Matches(symbol, &manager)) {
    // TODO: This is a simplified approach that assumes all nets are wires
    // In a real implementation, you'd need to check the actual net type
    const auto identifiers = GetIdentifiersFromNetDeclaration(symbol);
    for (const auto *token_info : identifiers) {
      if (token_info) {
        const auto name = token_info->text();
        if (!(name.size() > 2 && name.substr(0, 2) == "w_")) {
          Violation("wire", *token_info, context);
        }
      }
    }
    return;
  }

  // Handle module port declarations (inside module body)
  if (ModulePortMatcher().Matches(symbol, &manager)) {
    // Get direction
    const auto *direction_symbol = verible::GetSubtreeAsSymbol(
        symbol, NodeEnum::kModulePortDeclaration, 0);
    const auto direction =
        direction_symbol
            ? verible::SymbolCastToLeaf(*direction_symbol).get().text()
            : "input";

    // Get identifiers
    std::vector<const verible::TokenInfo *> identifiers;
    auto id_unpacked_dims =
        verible::SearchSyntaxTree(symbol, NodekIdentifierUnpackedDimensions());
    for (const auto &match : id_unpacked_dims) {
      const auto *leaf =
          GetSymbolIdentifierFromIdentifierUnpackedDimensions(*match.match);
      if (leaf) identifiers.push_back(&leaf->get());
    }
    if (identifiers.empty()) {
      auto port_ids = verible::SearchSyntaxTree(symbol, NodekPortIdentifier());
      for (const auto &match : port_ids) {
        const auto *leaf = GetIdentifier(*match.match);
        if (leaf) identifiers.push_back(&leaf->get());
      }
    }
    if (identifiers.empty()) {
      auto port_ids = verible::SearchSyntaxTree(symbol, NodekPortIdentifier());
      for (const auto &match : port_ids) {
        const auto *leaf = GetIdentifier(*match.match);
        if (leaf) identifiers.push_back(&leaf->get());
      }
    }

    // For each identifier, check prefix
    for (const auto *token : identifiers) {
      if (!token) continue;
      const auto name = token->text();
      bool has_correct_prefix = false;
      bool has_expected_prefix = false;

      if (name.size() > 2 && name.substr(0, 2) == "i_") {
        has_expected_prefix = true;
        has_correct_prefix = IsPrefixCorrect("i_", direction);
      } else if (name.size() > 2 && name.substr(0, 2) == "o_") {
        has_expected_prefix = true;
        has_correct_prefix = IsPrefixCorrect("o_", direction);
      } else if (name.size() > 3 && name.substr(0, 3) == "io_") {
        has_expected_prefix = true;
        has_correct_prefix = IsPrefixCorrect("io_", direction);
      }

      if (!has_expected_prefix || !has_correct_prefix) {
        Violation(direction, *token, context);
      }
    }
    return;
  }

  // Handle register variables (reg)
  const auto register_variables = FindAllRegisterVariables(symbol);
  for (const auto &match : register_variables) {
    if (match.match) {
      const auto *identifier_leaf = GetNameLeafOfRegisterVariable(*match.match);
      if (identifier_leaf) {
        const auto token = identifier_leaf->get();
        const auto name = token.text();

        if (!(name.size() > 2 && name.substr(0, 2) == "r_")) {
          Violation("reg", token, context);
        }
      }
    }
  }
}

LintRuleStatus SignalNamePrefixRule::Report() const {
  return LintRuleStatus(violations_, GetDescriptor());
}

}  // namespace analysis
}  // namespace verilog