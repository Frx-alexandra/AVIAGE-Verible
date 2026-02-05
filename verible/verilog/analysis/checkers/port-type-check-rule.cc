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

#include "verible/verilog/analysis/checkers/port-type-check-rule.h"

#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "verible/common/analysis/lint-rule-status.h"
#include "verible/common/analysis/matcher/bound-symbol-manager.h"
#include "verible/common/analysis/matcher/matcher.h"
#include "verible/common/text/symbol.h"
#include "verible/common/text/syntax-tree-context.h"
#include "verible/common/text/token-info.h"
#include "verible/common/text/tree-utils.h"
#include "verible/common/util/logging.h"
#include "verible/verilog/CST/port.h"
#include "verible/verilog/CST/verilog-matchers.h"
#include "verible/verilog/CST/verilog-nonterminals.h"
#include "verible/verilog/analysis/descriptions.h"
#include "verible/verilog/analysis/lint-rule-registry.h"
#include "verible/verilog/parser/verilog-token-enum.h"

namespace verilog {
namespace analysis {

using verible::LintRuleStatus;
using verible::LintViolation;
using verible::Symbol;
using verible::SyntaxTreeContext;
using verible::SyntaxTreeLeaf;
using verible::SyntaxTreeNode;
using verible::TokenInfo;
using verible::matcher::Matcher;

// Register PortTypeCheckRule.
VERILOG_REGISTER_LINT_RULE(PortTypeCheckRule);

static constexpr std::string_view kMessageInput =
    "input port must be wire type";
static constexpr std::string_view kMessageOutput =
    "output port must be wire or reg type";
static constexpr std::string_view kMessageInOut =
    "inout port must be wire type";

const LintRuleDescriptor &PortTypeCheckRule::GetDescriptor() {
  static const LintRuleDescriptor d{
      .name = "port-type-check",
      .topic = "port-types",
      .desc =
          "Check that port types are valid for their direction. "
          "Input ports must be wire type. "
          "Output ports must be wire or reg type. "
          "Inout ports must be wire type.",
  };
  return d;
}

static const Matcher &PortMatcher() {
  static const Matcher matcher(NodekPortDeclaration());
  return matcher;
}

// Helper function to extract the net/var type token from port declaration
// Returns nullptr if no explicit net/var type is specified
static const SyntaxTreeLeaf *GetNetVarTypeFromPortDeclaration(
    const Symbol &symbol) {
  // Index 1 in kPortDeclaration is var_or_net_type_opt
  const auto *type_symbol =
      verible::GetSubtreeAsSymbol(symbol, NodeEnum::kPortDeclaration, 1);
  if (!type_symbol) return nullptr;

  // If it's a leaf, it's the net/var type token (e.g., "wire", "var")
  if (type_symbol->Kind() == verible::SymbolKind::kLeaf) {
    return &verible::SymbolCastToLeaf(*type_symbol);
  }
  return nullptr;
}

// Helper function to extract the data type from kDataType node
// Returns the type token (e.g., "logic", "reg") if found, nullptr otherwise
static const SyntaxTreeLeaf *GetTypeFromDataTypeNode(const Symbol &data_type) {
  // kDataType has children: qualifiers, base_type, delay, packed_dims
  // Index 1 is the base_type which could be kDataTypePrimitive or other types
  const auto *base_type =
      verible::GetSubtreeAsSymbol(data_type, NodeEnum::kDataType, 1);
  if (!base_type) return nullptr;

  // If base_type is a primitive node, extract the first child which should be
  // the type token
  if (base_type->Kind() == verible::SymbolKind::kNode) {
    const auto &base_node = verible::SymbolCastToNode(*base_type);
    if (base_node.size() > 0 && base_node[0] != nullptr) {
      const auto *primitive = base_node[0].get();
      if (primitive->Kind() == verible::SymbolKind::kLeaf) {
        return &verible::SymbolCastToLeaf(*primitive);
      }
    }
  }

  return nullptr;
}

// Helper function to extract the data type token from port declaration
// Returns the type token if found, nullptr otherwise
static const SyntaxTreeLeaf *GetDataTypeFromPortDeclaration(
    const Symbol &symbol) {
  // Index 2 in kPortDeclaration contains data type info
  const auto *data_type_symbol =
      verible::GetSubtreeAsSymbol(symbol, NodeEnum::kPortDeclaration, 2);
  if (!data_type_symbol) return nullptr;

  // The data_type_symbol could be:
  // 1. kDataTypeImplicitBasicIdDimensions - for primitive types like "logic a"
  // 2. kDataType - for other cases

  if (data_type_symbol->Kind() == verible::SymbolKind::kNode) {
    const auto &node = verible::SymbolCastToNode(*data_type_symbol);

    // Check if it's kDataTypeImplicitBasicIdDimensions
    if (node.MatchesTag(NodeEnum::kDataTypeImplicitBasicIdDimensions)) {
      // First child is kDataType
      if (node.size() > 0 && node[0] != nullptr) {
        const auto *data_type = node[0].get();
        if (data_type->Kind() == verible::SymbolKind::kNode &&
            verible::SymbolCastToNode(*data_type)
                .MatchesTag(NodeEnum::kDataType)) {
          return GetTypeFromDataTypeNode(*data_type);
        }
      }
    }
    // Check if it's directly kDataType
    else if (node.MatchesTag(NodeEnum::kDataType)) {
      return GetTypeFromDataTypeNode(*data_type_symbol);
    }
  }

  return nullptr;
}

void PortTypeCheckRule::CheckPortType(std::string_view direction,
                                      const TokenInfo &type_token,
                                      const TokenInfo &identifier_token,
                                      const SyntaxTreeContext &context) {
  const auto type_text = type_token.text();

  if (direction == "input") {
    if (type_text != "wire") {
      violations_.insert(
          LintViolation(identifier_token, kMessageInput, context));
    }
  } else if (direction == "output") {
    if (type_text != "wire" && type_text != "reg") {
      violations_.insert(
          LintViolation(identifier_token, kMessageOutput, context));
    }
  } else if (direction == "inout") {
    if (type_text != "wire") {
      violations_.insert(
          LintViolation(identifier_token, kMessageInOut, context));
    }
  }
}

bool PortTypeCheckRule::IsValidTypeForDirection(std::string_view direction,
                                                std::string_view type) {
  if (direction == "input" || direction == "inout") {
    return type == "wire";
  } else if (direction == "output") {
    return type == "wire" || type == "reg";
  }
  return true;
}

void PortTypeCheckRule::HandleSymbol(const Symbol &symbol,
                                     const SyntaxTreeContext &context) {
  constexpr std::string_view implicit_direction = "input";
  verible::matcher::BoundSymbolManager manager;

  if (PortMatcher().Matches(symbol, &manager)) {
    const auto *identifier_leaf = GetIdentifierFromPortDeclaration(symbol);
    const auto *direction_leaf = GetDirectionFromPortDeclaration(symbol);

    if (!identifier_leaf) return;

    const auto identifier_token = identifier_leaf->get();
    const auto direction =
        direction_leaf ? direction_leaf->get().text() : implicit_direction;

    // First check for explicit net/var type at index 1 (e.g., "input wire a")
    const auto *net_var_type = GetNetVarTypeFromPortDeclaration(symbol);
    if (net_var_type) {
      const auto type_token = net_var_type->get();
      const auto type_text = type_token.text();

      // "var" is not a specific type check we care about here
      // Only check if it's "wire" or "reg"
      if (type_text == "wire" || type_text == "reg") {
        if (!IsValidTypeForDirection(direction, type_text)) {
          CheckPortType(direction, type_token, identifier_token, context);
        }
      }
      return;
    }

    // Check for data type primitive (e.g., "input logic a", "input reg a")
    const auto *data_type = GetDataTypeFromPortDeclaration(symbol);
    if (data_type) {
      const auto type_token = data_type->get();
      const auto type_text = type_token.text();

      // Check if this is a valid type for the direction
      if (!IsValidTypeForDirection(direction, type_text)) {
        CheckPortType(direction, type_token, identifier_token, context);
      }
      return;
    }

    // If no explicit type is found, the port uses implicit type
    // In Verilog, implicit type for input/inout is wire, for output it can be
    // wire This is typically considered valid, so we don't report a violation
  }
}

LintRuleStatus PortTypeCheckRule::Report() const {
  return LintRuleStatus(violations_, GetDescriptor());
}

}  // namespace analysis
}  // namespace verilog
