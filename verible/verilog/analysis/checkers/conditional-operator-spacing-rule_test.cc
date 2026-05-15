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

#include "verible/verilog/analysis/checkers/conditional-operator-spacing-rule.h"

#include <initializer_list>

#include "gtest/gtest.h"
#include "verible/common/analysis/linter-test-utils.h"
#include "verible/common/analysis/text-structure-linter-test-utils.h"
#include "verible/verilog/analysis/verilog-analyzer.h"
#include "verible/verilog/parser/verilog-token-enum.h"

namespace verilog {
namespace analysis {
namespace {

using verible::LintTestCase;
using verible::RunLintTestCases;

// Tests that properly spaced operators pass
TEST(ConditionalOperatorSpacingRuleTest, AcceptsEqualSpacing) {
  const std::initializer_list<LintTestCase> kTestCases = {
      // Equal comparison with spaces on both sides
      {"module m; assign x = (a == b); endmodule"},
      // Equal comparison with no spaces
      {"module m; assign x = (a==b); endmodule"},
      // Not equal with spaces
      {"module m; assign x = (a != b); endmodule"},
      // Not equal with no spaces
      {"module m; assign x = (a!=b); endmodule"},
      // Less than with spaces
      {"module m; assign x = (a < b); endmodule"},
      // Less than with no spaces
      {"module m; assign x = (a<b); endmodule"},
      // Greater than with spaces
      {"module m; assign x = (a > b); endmodule"},
      // Greater than with no spaces
      {"module m; assign x = (a>b); endmodule"},
      // Less than or equal with spaces
      {"module m; assign x = (a <= b); endmodule"},
      // Less than or equal with no spaces
      {"module m; assign x = (a<=b); endmodule"},
      // Greater than or equal with spaces
      {"module m; assign x = (a >= b); endmodule"},
      // Greater than or equal with no spaces
      {"module m; assign x = (a>=b); endmodule"},
      // Case equality with spaces
      {"module m; assign x = (a === b); endmodule"},
      // Case equality with no spaces
      {"module m; assign x = (a===b); endmodule"},
      // Case inequality with spaces
      {"module m; assign x = (a !== b); endmodule"},
      // Case inequality with no spaces
      {"module m; assign x = (a!==b); endmodule"},
      // Multiple operators with consistent spacing
      {"module m; assign x = (a == b && c != d); endmodule"},
      // Multiple operators without spaces
      {"module m; assign x = (a==b && c!=d); endmodule"},
      // If statement with comparison
      {"module m; always @(*) if (a == b) x = 1; endmodule"},
      // Complex expression with consistent spacing
      {"module m; assign x = (a < b) && (c > d) && (e == f); endmodule"},
  };
  RunLintTestCases<VerilogAnalyzer, ConditionalOperatorSpacingRule>(kTestCases);
}

// Tests that unequal spacing is rejected
TEST(ConditionalOperatorSpacingRuleTest, RejectsUnequalSpacing) {
  constexpr int kToken = TK_OTHER;
  const std::initializer_list<LintTestCase> kTestCases = {
      // Space before but not after ==
      {"module m; assign x = (a ", {kToken, "=="}, "b); endmodule"},
      // Space after but not before ==
      {"module m; assign x = (a", {kToken, "=="}, " b); endmodule"},
      // Space before but not after !=
      {"module m; assign x = (a ", {kToken, "!="}, "b); endmodule"},
      // Space after but not before !=
      {"module m; assign x = (a", {kToken, "!="}, " b); endmodule"},
      // Space before but not after <
      {"module m; assign x = (a ", {kToken, "<"}, "b); endmodule"},
      // Space after but not before <
      {"module m; assign x = (a", {kToken, "<"}, " b); endmodule"},
      // Space before but not after >
      {"module m; assign x = (a ", {kToken, ">"}, "b); endmodule"},
      // Space after but not before >
      {"module m; assign x = (a", {kToken, ">"}, " b); endmodule"},
      // Space before but not after <=
      {"module m; assign x = (a ", {kToken, "<="}, "b); endmodule"},
      // Space after but not before <=
      {"module m; assign x = (a", {kToken, "<="}, " b); endmodule"},
      // Space before but not after >=
      {"module m; assign x = (a ", {kToken, ">="}, "b); endmodule"},
      // Space after but not before >=
      {"module m; assign x = (a", {kToken, ">="}, " b); endmodule"},
      // Space before but not after ===
      {"module m; assign x = (a ", {kToken, "==="}, "b); endmodule"},
      // Space after but not before ===
      {"module m; assign x = (a", {kToken, "==="}, " b); endmodule"},
      // Space before but not after !==
      {"module m; assign x = (a ", {kToken, "!=="}, "b); endmodule"},
      // Space after but not before !==
      {"module m; assign x = (a", {kToken, "!=="}, " b); endmodule"},
  };
  RunLintTestCases<VerilogAnalyzer, ConditionalOperatorSpacingRule>(kTestCases);
}

// Tests multi-space scenarios
TEST(ConditionalOperatorSpacingRuleTest, AcceptsMultipleSpaces) {
  const std::initializer_list<LintTestCase> kTestCases = {
      // Two spaces on both sides
      {"module m; assign x = (a  ==  b); endmodule"},
      // Three spaces on both sides
      {"module m; assign x = (a   <   b); endmodule"},
  };
  RunLintTestCases<VerilogAnalyzer, ConditionalOperatorSpacingRule>(kTestCases);
}

// Tests unequal multi-space scenarios (should fail)
TEST(ConditionalOperatorSpacingRuleTest, RejectsUnequalMultipleSpaces) {
  constexpr int kToken = TK_OTHER;
  const std::initializer_list<LintTestCase> kTestCases = {
      // One space before, two spaces after
      {"module m; assign x = (a ", {kToken, "=="}, "  b); endmodule"},
      // Two spaces before, one space after
      {"module m; assign x = (a  ", {kToken, "=="}, " b); endmodule"},
  };
  RunLintTestCases<VerilogAnalyzer, ConditionalOperatorSpacingRule>(kTestCases);
}

// Tests that newlines don't trigger violations
TEST(ConditionalOperatorSpacingRuleTest, AcceptsNewlines) {
  const std::initializer_list<LintTestCase> kTestCases = {
      // Operator on new line
      {"module m;\n"
       "  assign x = (a\n"
       "    == b);\n"
       "endmodule"},
      // Operand on new line after operator
      {"module m;\n"
       "  assign x = (a ==\n"
       "    b);\n"
       "endmodule"},
  };
  RunLintTestCases<VerilogAnalyzer, ConditionalOperatorSpacingRule>(kTestCases);
}

// Tests mixed valid and invalid spacing
TEST(ConditionalOperatorSpacingRuleTest, MixedSpacing) {
  constexpr int kToken = TK_OTHER;
  const std::initializer_list<LintTestCase> kTestCases = {
      // First operator good, second bad
      {"module m; assign x = (a == b && c ", {kToken, "!="}, "d); endmodule"},
  };
  RunLintTestCases<VerilogAnalyzer, ConditionalOperatorSpacingRule>(kTestCases);
}

// Tests operators in different contexts
TEST(ConditionalOperatorSpacingRuleTest, DifferentContexts) {
  const std::initializer_list<LintTestCase> kTestCases = {
      // In if statement
      {"module m; always @(*) if (a == b) x = 1; endmodule"},
      // In while loop
      {"module m; initial while (a < b) x = x + 1; endmodule"},
      // In for loop
      {"module m; initial for (i=0; i < 10; i=i+1) x = x + 1; endmodule"},
      // In case statement  
      {"module m; always @(*) case (1) (a==b): x=1; endcase endmodule"},
      // In ternary operator
      {"module m; assign x = (a == b) ? 1 : 0; endmodule"},
  };
  RunLintTestCases<VerilogAnalyzer, ConditionalOperatorSpacingRule>(kTestCases);
}

// Tests that non-blocking assignments with <= are not flagged
TEST(ConditionalOperatorSpacingRuleTest, AcceptsNonBlockingAssignments) {
  const std::initializer_list<LintTestCase> kTestCases = {
      // Simple non-blocking assignment with spaces
      {"module m; always @(posedge clk) buffer <= data_in; endmodule"},
      // Non-blocking assignment without spaces
      {"module m; always @(posedge clk) buffer<=data_in; endmodule"},
      // Non-blocking assignment with unequal spaces (should be accepted for assignments)
      {"module m; always @(posedge clk) buffer <=  data_in; endmodule"},
      {"module m; always @(posedge clk) buffer<=  data_in; endmodule"},
      // Non-blocking assignment in if-else
      {"module m;\n"
       "  always @(posedge clk) begin\n"
       "    if (!rst_n) begin\n"
       "      buffer <= 8'b0;\n"
       "    end else begin\n"
       "      buffer <= data_in;\n"
       "    end\n"
       "  end\n"
       "endmodule"},
      // Multiple non-blocking assignments
      {"module m;\n"
       "  always @(posedge clk) begin\n"
       "    a <= b;\n"
       "    c <= d;\n"
       "    e <= f;\n"
       "  end\n"
       "endmodule"},
      // Non-blocking assignment with array indexing
      {"module m; always @(posedge clk) buffer[i] <= data_in; endmodule"},
      // Blocking assignment with = (should not be affected)
      {"module m; always @(*) buffer = data_in; endmodule"},
  };
  RunLintTestCases<VerilogAnalyzer, ConditionalOperatorSpacingRule>(kTestCases);
}

// Tests that <= in comparison context is still checked
TEST(ConditionalOperatorSpacingRuleTest, ComparisonOperatorsStillChecked) {
  constexpr int kToken = TK_OTHER;
  const std::initializer_list<LintTestCase> kTestCases = {
      // <= in if condition should still be checked
      {"module m; always @(*) if (a ", {kToken, "<="}, "b) x = 1; endmodule"},
      // <= in while condition should still be checked  
      {"module m; initial while (a", {kToken, "<="}, " b) x = x + 1; endmodule"},
      // <= in parentheses (expression) should still be checked
      {"module m; assign x = (a ", {kToken, "<="}, "b); endmodule"},
      {"module m; assign x = (a", {kToken, "<="}, " b); endmodule"},
      // >= should always be checked (it's never a non-blocking assignment)
      {"module m; assign x = (a ", {kToken, ">="}, "b); endmodule"},
      {"module m; assign x = (a", {kToken, ">="}, " b); endmodule"},
  };
  RunLintTestCases<VerilogAnalyzer, ConditionalOperatorSpacingRule>(kTestCases);
}

// Tests multi-line conditions with comparison operators
TEST(ConditionalOperatorSpacingRuleTest, MultiLineConditions) {
  constexpr int kToken = TK_OTHER;
  const std::initializer_list<LintTestCase> kTestCases = {
      // Multi-line if with <= should still be checked (inside parentheses)
      {"module m;\n"
       "  always @(*) if (\n"
       "    a ", {kToken, "<="}, "b\n"
       "  ) x = 1;\n"
       "endmodule"},
      // Multi-line condition with proper spacing should pass
      {"module m;\n"
       "  always @(*) if (\n"
       "    a <= b\n"
       "  ) x = 1;\n"
       "endmodule"},
      // Multi-line while with comparison
      {"module m;\n"
       "  initial while (\n"
       "    count < max\n"
       "  ) count = count + 1;\n"
       "endmodule"},
  };
  RunLintTestCases<VerilogAnalyzer, ConditionalOperatorSpacingRule>(kTestCases);
}

}  // namespace
}  // namespace analysis
}  // namespace verilog
