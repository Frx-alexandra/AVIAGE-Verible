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

#include "verible/verilog/analysis/checkers/forbidden-name-constant-with-value-rule.h"

#include <cctype>
#include <iomanip>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <string_view>

#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "verible/common/analysis/lint-rule-status.h"
#include "verible/common/text/token-info.h"
#include "verible/verilog/analysis/descriptions.h"
#include "verible/verilog/analysis/lint-rule-registry.h"
#include "verible/verilog/parser/verilog-lexer.h"
#include "verible/verilog/parser/verilog-token-classifications.h"
#include "verible/verilog/parser/verilog-token-enum.h"

namespace verilog {
namespace analysis {

VERILOG_REGISTER_LINT_RULE(ForbiddenNameConstantWithValueRule);

using verible::LintRuleStatus;
using verible::LintViolation;
using verible::TokenInfo;

ForbiddenNameConstantWithValueRule::ForbiddenNameConstantWithValueRule()
    : current_macro_token_(verible::TokenInfo::EOFToken()),
      base_for_literal_(10) {}

const LintRuleDescriptor &ForbiddenNameConstantWithValueRule::GetDescriptor() {
  static const LintRuleDescriptor d{
      .name = "forbidden-name-constant-with-value",
      .topic = "defines",
      .desc =
          "Checks that macro constants defined with `define do not have names "
          "that contain the numeric value being defined. For example, "
          "`define C_1001 4'b1001 is discouraged while `define C_CTRL_ADDR "
          "4'b1001 is preferred. The rule detects decimal, binary (e.g., "
          "4'b1001), "
          "and hexadecimal (e.g., 8'hFF) numeric literals. No configuration "
          "parameters are required.",
      .param = {},
  };
  return d;
}

// Extract numeric value from various formats:
// - Decimal: 42, 1001
// - Binary: 4'b1001, 4'b0001, 'b1010
// - Hexadecimal: 8'hFF, 8'h0F, 'hABCD
// - Octal: 3'o7
// - Digit-only tokens: "1010" with base_hint=2
// If base_hint is provided, it overrides auto-detection
// Returns true if a numeric value was extracted
bool ForbiddenNameConstantWithValueRule::ExtractNumericValue(
    std::string_view text, int64_t *value, int base_hint) {
  std::string num_str(text);

  // Remove underscores that are used as digit separators
  num_str.erase(std::remove(num_str.begin(), num_str.end(), '_'),
                num_str.end());

  // If base_hint is provided, parse directly with that base
  if (base_hint > 0) {
    try {
      size_t pos = 0;
      *value = std::stoll(num_str, &pos, base_hint);
      return pos == num_str.size();
    } catch (...) {
      return false;
    }
  }

  // Check for sized/unsized literals: <size>'<base><value>
  // or just <base><value> for unsized
  size_t quote_pos = num_str.find('\'');
  if (quote_pos != std::string::npos) {
    // Has a base specifier
    if (quote_pos + 1 >= num_str.size()) return false;

    char base_char = std::tolower(num_str[quote_pos + 1]);
    std::string value_part = num_str.substr(quote_pos + 2);

    if (value_part.empty()) return false;

    try {
      switch (base_char) {
        case 'b':
          *value = std::stoll(value_part, nullptr, 2);
          return true;
        case 'o':
          *value = std::stoll(value_part, nullptr, 8);
          return true;
        case 'd':
          *value = std::stoll(value_part, nullptr, 10);
          return true;
        case 'h':
          *value = std::stoll(value_part, nullptr, 16);
          return true;
        default:
          return false;
      }
    } catch (...) {
      return false;
    }
  } else {
    // Plain decimal number
    try {
      size_t pos = 0;
      *value = std::stoll(num_str, &pos, 10);
      // Make sure we consumed the entire string
      return pos == num_str.size();
    } catch (...) {
      return false;
    }
  }
}

// Check if macro name contains the value in specific bases
// bases: bitwise OR of which bases to check (1=binary, 8=octal, 10=decimal,
// 16=hex)
bool ForbiddenNameConstantWithValueRule::NameContainsValue(
    const std::string &name, int64_t value, int bases_to_check) {
  std::string upper_name = name;
  std::transform(upper_name.begin(), upper_name.end(), upper_name.begin(),
                 ::toupper);

  // Check decimal representation if requested
  if (bases_to_check & 10) {  // decimal = 10
    std::string dec_str = std::to_string(value);
    std::string upper_dec = dec_str;
    std::transform(upper_dec.begin(), upper_dec.end(), upper_dec.begin(),
                   ::toupper);
    if (upper_name.find(upper_dec) != std::string::npos) {
      return true;
    }
    // Also check with underscore separators (e.g., 1_000_000)
    std::string dec_with_underscores = dec_str;
    // Add underscores every 3 digits from the right
    int pos = dec_with_underscores.length() - 3;
    while (pos > 0) {
      dec_with_underscores.insert(pos, "_");
      pos -= 3;
    }
    std::string upper_dec_us = dec_with_underscores;
    std::transform(upper_dec_us.begin(), upper_dec_us.end(),
                   upper_dec_us.begin(), ::toupper);
    if (upper_name.find(upper_dec_us) != std::string::npos) {
      return true;
    }
  }

  // Check hex representation if requested
  if (bases_to_check & 16) {  // hex = 16
    std::stringstream hex_stream;
    hex_stream << std::hex << std::uppercase << value;
    std::string hex_upper = hex_stream.str();
    if (upper_name.find(hex_upper) != std::string::npos) {
      return true;
    }
    // Also check with leading zeros removed
    size_t first_non_zero = hex_upper.find_first_not_of('0');
    if (first_non_zero != std::string::npos && first_non_zero > 0) {
      std::string hex_no_leading = hex_upper.substr(first_non_zero);
      if (upper_name.find(hex_no_leading) != std::string::npos) {
        return true;
      }
    }
  }

  // Check binary representation if requested
  if (bases_to_check & 1) {  // binary = 1
    if (value >= 0) {
      std::string binary;
      int64_t temp = value;
      if (temp == 0) {
        binary = "0";
      } else {
        while (temp > 0) {
          binary = char('0' + (temp & 1)) + binary;
          temp >>= 1;
        }
      }
      if (upper_name.find(binary) != std::string::npos) {
        return true;
      }

      // Also check padded versions (4, 8, 16, 32 bits)
      for (int bits : {4, 8, 16, 32}) {
        if (value < (1LL << bits)) {
          std::string padded_binary;
          for (int i = bits - 1; i >= 0; --i) {
            padded_binary += char('0' + ((value >> i) & 1));
          }
          if (upper_name.find(padded_binary) != std::string::npos) {
            return true;
          }
        }
      }
    }
  }

  // Check octal representation if requested
  if (bases_to_check & 8) {  // octal = 8
    std::stringstream oct_stream;
    oct_stream << std::oct << value;
    std::string octal = oct_stream.str();
    std::string upper_oct = octal;
    std::transform(upper_oct.begin(), upper_oct.end(), upper_oct.begin(),
                   ::toupper);
    if (upper_name.find(upper_oct) != std::string::npos) {
      return true;
    }
  }

  return false;
}

void ForbiddenNameConstantWithValueRule::HandleToken(const TokenInfo &token) {
  const auto token_enum = static_cast<verilog_tokentype>(token.token_enum());
  const std::string_view text(token.text());

  if (IsUnlexed(verilog_tokentype(token.token_enum()))) {
    // recursively lex to examine inside macro definition bodies, etc.
    RecursiveLexText(
        text, [this](const TokenInfo &subtoken) { HandleToken(subtoken); });
    return;
  }

  switch (state_) {
    case State::kNormal: {
      // Only changes state on `define tokens
      switch (token_enum) {
        case PP_define:
          state_ = State::kExpectPPIdentifier;
          break;
        default:
          break;
      }
      break;
    }
    case State::kExpectPPIdentifier: {
      switch (token_enum) {
        case TK_SPACE:  // stay in the same state
          break;
        case PP_Identifier: {
          // Store the macro name and move to checking for value
          current_macro_name_ = std::string(text);
          current_macro_token_ = token;
          state_ = State::kExpectMacroValue;
          break;
        }
        default:
          // Unexpected token, reset
          state_ = State::kNormal;
          break;
      }
      break;
    }
    case State::kExpectMacroValue: {
      // After macro name, we look for either:
      // 1. TK_SPACE/TK_NEWLINE - continue waiting for value or end
      // 2. Macro call parameters ( '(' ) - parameterized macro, skip
      // 3. Size prefix like '4' before a based literal
      // 4. Base specifier like 'b', 'h', 'o', 'd'
      // 5. Digits tokens (TK_*Digits)
      // 6. Plain decimal number
      switch (token_enum) {
        case TK_SPACE:
        case TK_NEWLINE:
        case TK_LINE_CONT:
          // Stay in this state, waiting for actual value
          break;
        case TK_BinBase:
          // Saw 'b after optional size - binary literal coming
          base_for_literal_ = 2;
          break;
        case TK_HexBase:
          // Saw 'h after optional size - hex literal coming
          base_for_literal_ = 16;
          break;
        case TK_OctBase:
          // Saw 'o after optional size - octal literal coming
          base_for_literal_ = 8;
          break;
        case TK_DecBase:
          // Saw 'd after optional size - decimal literal coming
          base_for_literal_ = 10;
          break;
        case TK_BinDigits: {
          // Binary digits - check both binary and decimal representations
          int64_t value;
          if (ExtractNumericValue(text, &value, 2)) {
            // Check binary (1) and decimal (10) representations
            if (NameContainsValue(current_macro_name_, value, 1 | 10)) {
              violations_.insert(LintViolation(
                  current_macro_token_,
                  absl::StrCat("Macro constant name '", current_macro_name_,
                               "' contains the value ", value,
                               ". Use a descriptive name instead.")));
            }
          }
          state_ = State::kNormal;
          base_for_literal_ = 10;
          current_macro_name_.clear();
          break;
        }
        case TK_HexDigits: {
          // Hex digits - check both hex and decimal representations
          int64_t value;
          if (ExtractNumericValue(text, &value, 16)) {
            // Check hex (16) and decimal (10) representations
            if (NameContainsValue(current_macro_name_, value, 16 | 10)) {
              violations_.insert(LintViolation(
                  current_macro_token_,
                  absl::StrCat("Macro constant name '", current_macro_name_,
                               "' contains the value ", value,
                               ". Use a descriptive name instead.")));
            }
          }
          state_ = State::kNormal;
          base_for_literal_ = 10;
          current_macro_name_.clear();
          break;
        }
        case TK_OctDigits: {
          // Octal digits - check both octal and decimal representations
          int64_t value;
          if (ExtractNumericValue(text, &value, 8)) {
            // Check octal (8) and decimal (10) representations
            if (NameContainsValue(current_macro_name_, value, 8 | 10)) {
              violations_.insert(LintViolation(
                  current_macro_token_,
                  absl::StrCat("Macro constant name '", current_macro_name_,
                               "' contains the value ", value,
                               ". Use a descriptive name instead.")));
            }
          }
          state_ = State::kNormal;
          base_for_literal_ = 10;
          current_macro_name_.clear();
          break;
        }
        case TK_DecNumber:
          // This could be:
          // 1. A plain decimal value (we should check it)
          // 2. The size prefix of a based literal (we should wait for digits)
          // We can't tell yet, so just remember we saw it and check later
          // For now, assume it's a plain decimal and check it (decimal only)
          {
            int64_t value;
            if (ExtractNumericValue(text, &value)) {
              if (NameContainsValue(current_macro_name_, value,
                                    10)) {  // decimal only
                violations_.insert(LintViolation(
                    current_macro_token_,
                    absl::StrCat("Macro constant name '", current_macro_name_,
                                 "' contains the decimal value ", value,
                                 ". Use a descriptive name instead.")));
              }
            }
          }
          // Don't clear yet - this might be a size prefix
          // We'll clear when we see the actual digits or a non-literal token
          break;
        case TK_UnBasedNumber: {
          // Unsized literal like '1 or '0 - check decimal only
          int64_t value;
          if (ExtractNumericValue(text, &value)) {
            if (NameContainsValue(current_macro_name_, value,
                                  10)) {  // decimal only
              violations_.insert(LintViolation(
                  current_macro_token_,
                  absl::StrCat("Macro constant name '", current_macro_name_,
                               "' contains the decimal value ", value,
                               ". Use a descriptive name instead.")));
            }
          }
          state_ = State::kNormal;
          current_macro_name_.clear();
          break;
        }
        default:
          // Check if this is an open parenthesis (parameterized macro)
          if (text == "(") {
            // This is a parameterized macro (like `define FOO(a,b) ...)
            // Skip checking for this macro
            state_ = State::kNormal;
            current_macro_name_.clear();
          } else {
            // For other non-numeric tokens, just reset state
            state_ = State::kNormal;
            current_macro_name_.clear();
          }
          break;
      }
      break;
    }
  }
}

absl::Status ForbiddenNameConstantWithValueRule::Configure(
    std::string_view configuration) {
  // No configuration needed for this rule
  return absl::OkStatus();
}

LintRuleStatus ForbiddenNameConstantWithValueRule::Report() const {
  return LintRuleStatus(violations_, GetDescriptor());
}

}  // namespace analysis
}  // namespace verilog
