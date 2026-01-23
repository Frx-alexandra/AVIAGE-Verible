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

#include "verible/verilog/analysis/checkers/packed-dimensions-range-decrease-to-zero-rule.h"

#include <set>
#include <string_view>

#include "verible/common/analysis/lint-rule-status.h"
#include "verible/common/analysis/matcher/bound-symbol-manager.h"
#include "verible/common/analysis/matcher/matcher.h"
#include "verible/common/text/symbol.h"
#include "verible/common/text/syntax-tree-context.h"
#include "verible/common/text/token-info.h"
#include "verible/common/text/tree-utils.h"
#include "verible/common/util/logging.h"
#include "verible/verilog/CST/context-functions.h"
#include "verible/verilog/CST/declaration.h"
#include "verible/verilog/CST/dimensions.h"
#include "verible/verilog/CST/expression.h"
#include "verible/verilog/CST/net.h"
#include "verible/verilog/CST/type.h"
#include "verible/verilog/CST/verilog-matchers.h"
#include "verible/verilog/analysis/descriptions.h"
#include "verible/verilog/analysis/lint-rule-registry.h"
#include "verible/verilog/parser/verilog-token-enum.h"

namespace verilog {
namespace analysis {

using verible::LintRuleStatus;
using verible::LintViolation;
using verible::matcher::Matcher;

VERILOG_REGISTER_LINT_RULE(PackedDimensionsRangeDecreaseToZeroRule);

static constexpr std::string_view kMessage =
    "Declare packed dimension range for reg/wire variables with length > 1 in "
    "little-endian (decreasing) order with right-hand side 0, e.g. [N-1:0].";

const LintRuleDescriptor &
PackedDimensionsRangeDecreaseToZeroRule::GetDescriptor() {
  static const LintRuleDescriptor d{
      .name = "packed-dimensions-range-decrease-to-zero",
      .topic = "packed-ordering",
      .desc =
          "Checks that packed dimension ranges for reg and wire variables with "
          "length > 1 are declared in little-endian (decreasing) order and "
          "have "
          "right-hand side of 0, e.g. `[N-1:0]`.",
  };
  return d;
}

static const Matcher &DimensionRangeMatcher() {
  static const Matcher matcher(NodekDimensionRange());
  return matcher;
}

// Helper function to check if the current context is within a wire or reg
// declaration
static bool IsWithinWireOrRegDeclaration(
    const verible::SyntaxTreeContext &context) {
  // Walk up the context to find if we're in a net declaration or data
  // declaration
  for (const auto &ancestor : context) {
    if (ancestor->Tag().tag == static_cast<int>(NodeEnum::kNetDeclaration)) {
      // Check if it's a wire
      const auto *data_type =
          verible::GetSubtreeAsSymbol(*ancestor, NodeEnum::kNetDeclaration, 0);
      if (data_type &&
          data_type->Tag().tag == static_cast<int>(NodeEnum::kDataType)) {
        const auto *type_symbol =
            verible::GetSubtreeAsSymbol(*data_type, NodeEnum::kDataType, 1);
        if (type_symbol && type_symbol->Kind() == verible::SymbolKind::kLeaf) {
          const auto &leaf = verible::SymbolCastToLeaf(*type_symbol);
          if (leaf.get().text() == "wire") {
            return true;
          }
        }
      }
    } else if (ancestor->Tag().tag ==
               static_cast<int>(NodeEnum::kDataDeclaration)) {
      // Check if it's a reg
      const verible::SyntaxTreeNode *instantiation_type =
          GetInstantiationTypeOfDataDeclaration(*ancestor);
      if (instantiation_type) {
        const verible::SyntaxTreeNode *data_type = verible::GetSubtreeAsNode(
            *instantiation_type, NodeEnum::kInstantiationType, 0,
            NodeEnum::kDataType);
        if (data_type) {
          const verible::Symbol *base_type =
              GetBaseTypeFromDataType(*data_type);
          if (base_type && base_type->Tag().tag ==
                               static_cast<int>(NodeEnum::kDataTypePrimitive)) {
            const verible::Symbol *type_leaf = verible::GetSubtreeAsSymbol(
                *base_type, NodeEnum::kDataTypePrimitive, 0);
            if (type_leaf && type_leaf->Kind() == verible::SymbolKind::kLeaf) {
              const auto &leaf = verible::SymbolCastToLeaf(*type_leaf);
              if (leaf.get().text() == "reg") {
                return true;
              }
            }
          }
        }
      }
    } else if (ancestor->Tag().tag ==
               static_cast<int>(NodeEnum::kModulePortDeclaration)) {
      // Check if it's a wire in a module port
      // Module ports can have wire/reg type
      const auto *port_type = verible::GetSubtreeAsSymbol(
          *ancestor, NodeEnum::kModulePortDeclaration, 1);
      if (port_type &&
          port_type->Tag().tag == static_cast<int>(NodeEnum::kDataType)) {
        const auto *type_symbol =
            verible::GetSubtreeAsSymbol(*port_type, NodeEnum::kDataType, 0);
        if (type_symbol && type_symbol->Kind() == verible::SymbolKind::kLeaf) {
          const auto &leaf = verible::SymbolCastToLeaf(*type_symbol);
          if (leaf.get().text() == "wire") {
            return true;
          }
        }
      }
    }
  }
  return false;
}

void PackedDimensionsRangeDecreaseToZeroRule::HandleSymbol(
    const verible::Symbol &symbol, const verible::SyntaxTreeContext &context) {
  if (!ContextIsInsidePackedDimensions(context)) return;

  // Only apply to wire or reg declarations
  if (!IsWithinWireOrRegDeclaration(context)) return;

  verible::matcher::BoundSymbolManager manager;
  if (DimensionRangeMatcher().Matches(symbol, &manager)) {
    // Check whether or not bounds are numeric constants
    const auto &left = *ABSL_DIE_IF_NULL(GetDimensionRangeLeftBound(symbol));
    const auto &right = *ABSL_DIE_IF_NULL(GetDimensionRangeRightBound(symbol));
    int left_value, right_value;
    const bool left_is_constant = ConstantIntegerValue(left, &left_value);
    const bool right_is_constant = ConstantIntegerValue(right, &right_value);

    // Check if right bound is 0
    const bool right_is_zero = right_is_constant && (right_value == 0);

    // Check if length > 1 (left > right for descending order)
    const bool is_descending =
        left_is_constant && right_is_constant && left_value > right_value;

    // Only check for multi-bit signals (length > 1)
    if (!is_descending) return;

    // For multi-bit signals, flag if right is not 0
    if (!right_is_zero) {
      const verible::TokenInfo token(TK_OTHER,
                                     verible::StringSpanOfSymbol(left, right));
      violations_.insert(LintViolation(token, kMessage, context));
    }

    // If it's descending but right is not 0, or if right is 0 but not
    // descending (length <= 1), then it's a violation if it's supposed to be
    // [N-1:0] format
    if ((left_is_constant && right_is_constant && left_value > right_value &&
         !right_is_zero) ||
        (right_is_zero && (!left_is_constant || !right_is_constant ||
                           left_value <= right_value))) {
      const verible::TokenInfo token(TK_OTHER,
                                     verible::StringSpanOfSymbol(left, right));
      violations_.insert(LintViolation(token, kMessage, context));
    }
  }
}

LintRuleStatus PackedDimensionsRangeDecreaseToZeroRule::Report() const {
  return LintRuleStatus(violations_, GetDescriptor());
}

}  // namespace analysis
}  // namespace verilog