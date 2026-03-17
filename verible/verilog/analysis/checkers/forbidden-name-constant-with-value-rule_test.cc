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

#include <initializer_list>

#include "gtest/gtest.h"
#include "verible/common/analysis/linter-test-utils.h"
#include "verible/common/analysis/token-stream-linter-test-utils.h"
#include "verible/verilog/analysis/verilog-analyzer.h"
#include "verible/verilog/parser/verilog-token-enum.h"

namespace verilog {
namespace analysis {
namespace {

using verible::LintTestCase;
using verible::RunLintTestCases;

// Tests that the expected number of ForbiddenNameConstantWithValueRule
// violations are found.
TEST(ForbiddenNameConstantWithValueRuleTest, BinaryConstants) {
  constexpr int kToken = PP_Identifier;

  const std::initializer_list<LintTestCase> kTestCases = {
      // Legal - descriptive names
      {"`define C_CTRL_ADDR 4'b1001"},
      {"`define STATUS_REG 8'b10101010"},
      {"`define ENABLE_BIT 1'b1"},

      // Illegal - name contains the value
      {"`define ", {kToken, "C_1001"}, " 4'b1001"},
      {"`define ", {kToken, "VAL_1010"}, " 4'b1010"},
      {"`define ", {kToken, "REG_10101010"}, " 8'b10101010"},
      {"`define ", {kToken, "BIT_1"}, " 1'b1"},

      // Different values - should not match (only check binary repr)
      {"`define C_CTRL_ADDR 4'b1010"},  // "1010" not in "C_CTRL_ADDR"
  };
  RunLintTestCases<VerilogAnalyzer, ForbiddenNameConstantWithValueRule>(
      kTestCases);
}

TEST(ForbiddenNameConstantWithValueRuleTest, HexConstants) {
  constexpr int kToken = PP_Identifier;

  const std::initializer_list<LintTestCase> kTestCases = {
      // Legal - descriptive names
      {"`define STATUS_MASK 8'hFF"},
      {"`define ADDR_HIGH 16'hABCD"},
      {"`define LOW_NIBBLE 4'h0F"},

      // Illegal - name contains hex value
      {"`define ", {kToken, "MASK_FF"}, " 8'hFF"},
      {"`define ", {kToken, "VAL_ABCD"}, " 16'hABCD"},
      {"`define ", {kToken, "NIBBLE_0F"}, " 4'h0F"},

      // Hex in different formats
      {"`define ", {kToken, "HEX_FF"}, " 8'hff"},   // lowercase
      {"`define ", {kToken, "ADDR_FF"}, " 8'hFF"},  // uppercase
  };
  RunLintTestCases<VerilogAnalyzer, ForbiddenNameConstantWithValueRule>(
      kTestCases);
}

TEST(ForbiddenNameConstantWithValueRuleTest, DecimalConstants) {
  constexpr int kToken = PP_Identifier;

  const std::initializer_list<LintTestCase> kTestCases = {
      // Legal - descriptive names
      {"`define MAX_COUNT 100"},
      {"`define BUFFER_SIZE 4096"},

      // Illegal - name contains decimal value
      {"`define ", {kToken, "COUNT_100"}, " 100"},
      {"`define ", {kToken, "SIZE_4096"}, " 4096"},
      {"`define ", {kToken, "VAL_42"}, " 42"},
  };
  RunLintTestCases<VerilogAnalyzer, ForbiddenNameConstantWithValueRule>(
      kTestCases);
}

TEST(ForbiddenNameConstantWithValueRuleTest, ParameterizedMacrosIgnored) {
  // Parameterized macros should be ignored
  const std::initializer_list<LintTestCase> kTestCases = {
      {"`define ADD_1001(a,b) a+b"},      // parameterized, should be ignored
      {"`define MASK_FF(x) x & 8'hFF"},   // parameterized, should be ignored
      {"`define CALC_100(a,b,c) a+b+c"},  // parameterized, should be ignored
  };
  RunLintTestCases<VerilogAnalyzer, ForbiddenNameConstantWithValueRule>(
      kTestCases);
}

TEST(ForbiddenNameConstantWithValueRuleTest, ComplexExpressions) {
  constexpr int kToken = PP_Identifier;

  // Test with expressions and non-numeric values
  const std::initializer_list<LintTestCase> kTestCases = {
      // String values - no numeric check
      {"`define MSG_1001 \"message\""},
      {"`define NAME_CTRL_ADDR \"C_CTRL_ADDR\""},

      // Non-numeric identifiers
      {"`define WIDTH_ADDR width"},
      {"`define OTHER_VAL other"},

      // Value in comments or complex expressions - only first token checked
      {"`define ", {kToken, "VAL_100"}, " 100 + 1"},
  };
  RunLintTestCases<VerilogAnalyzer, ForbiddenNameConstantWithValueRule>(
      kTestCases);
}

TEST(ForbiddenNameConstantWithValueRuleTest, EdgeCases) {
  constexpr int kToken = PP_Identifier;

  const std::initializer_list<LintTestCase> kTestCases = {
      // Zero values
      {"`define ", {kToken, "VAL_0"}, " 4'b0000"},
      // Zero with decimal - name doesn't contain "0" so not a violation
      {"`define ZERO_VAL 0"},

      // Large numbers
      {"`define ", {kToken, "VAL_4294967295"}, " 32'hFFFFFFFF"},

      // Multiple representations possible - decimal in name for hex literal
      {"`define ", {kToken, "MASK_255"}, " 8'hFF"},  // 255 = 0xFF
      // For decimal literals, only check decimal representation
      {"`define MASK_FF 8'd255"},  // "255" not in "MASK_FF", so OK

      // Underscores in numbers
      {"`define ",
       {kToken, "VAL_1_000_000"},
       " 1_000_000"},  // with underscores
      {"`define ",
       {kToken, "VAL_1000000"},
       " 1_000_000"},  // without underscores
  };
  RunLintTestCases<VerilogAnalyzer, ForbiddenNameConstantWithValueRule>(
      kTestCases);
}

}  // namespace
}  // namespace analysis
}  // namespace verilog
