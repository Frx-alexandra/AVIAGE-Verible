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

#include "verible/verilog/analysis/checkers/begin-same-line-rule.h"

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

// Tests that space-only text passes.
TEST(BeginSameLineRuleTest, AcceptsBlank) {
  const std::initializer_list<LintTestCase> kTestCases = {
      {""},
      {" "},
      {"\n"},
      {" \n\n"},
  };
  RunLintTestCases<VerilogAnalyzer, BeginSameLineRule>(kTestCases);
}

// Tests that begin on same line with statements passes.
TEST(BeginSameLineRuleTest, AcceptsBeginOnSameLine) {
  const std::initializer_list<LintTestCase> kTestCases = {
      {"if (FOO) begin a <= 1; end"},
      {"else begin FOO end"},
      {"else if (FOO) begin a <= 1; end"},
      {"for(i = 0; i < N; i++) begin a <= 1; end"},
      {"foreach(array[i]) begin a <= 1; end"},
      {"while (a < 3) begin a = a + 1; end"},
      {"forever begin a <= 1; end"},
      {"initial begin a <= 1; end"},
      {"always_comb begin a = 1; end"},
      {"always_latch begin a <= 1; end"},
      {"always @ (a or b) begin a <= 1; end"},
  };
  RunLintTestCases<VerilogAnalyzer, BeginSameLineRule>(kTestCases);
}

// Tests that begin on different line with statements fails.
TEST(BeginSameLineRuleTest, RejectsBeginOnDifferentLine) {
  const std::initializer_list<LintTestCase> kTestCases = {
      {{TK_if, "if"}, " (FOO)\n begin a <= 1; end"},
      {{TK_else, "else"}, " \nbegin FOO end"},
      {"else ", {TK_if, "if"}, " (FOO)\n begin a <= 1; end"},
      {{TK_for, "for"}, "(i = 0; i < N; i++)\nbegin a <= 1; end"},
      {{TK_foreach, "foreach"}, "(array[i])\nbegin a <= 1; end"},
      {{TK_while, "while"}, " (a < 3)\nbegin a = a + 1; end"},
      {{TK_forever, "forever"}, "\nbegin a <= 1; end"},
      {{TK_initial, "initial"}, "\nbegin a <= 1; end"},
      {{TK_always_comb, "always_comb"}, "\nbegin a = 1; end"},
      {{TK_always_latch, "always_latch"}, "\nbegin a <= 1; end"},
      {{TK_always, "always"}, " @ (a or b)\nbegin a <= 1; end"},
  };
  RunLintTestCases<VerilogAnalyzer, BeginSameLineRule>(kTestCases);
}

// Tests that statements without begin don't trigger violations.
TEST(BeginSameLineRuleTest, AcceptsStatementsWithoutBegin) {
  const std::initializer_list<LintTestCase> kTestCases = {
      {"if (FOO) a <= 1;"},          {"else FOO"},
      {"else if (FOO) a <= 1;"},     {"for(i = 0; i < N; i++) a <= 1;"},
      {"foreach(array[i]) a <= 1;"}, {"while (a < 3) a = a + 1;"},
      {"forever a <= 1;"},           {"initial a <= 1;"},
      {"always_comb a = 1;"},        {"always_latch a <= 1;"},
      {"always @ (a or b) a <= 1;"},
  };
  RunLintTestCases<VerilogAnalyzer, BeginSameLineRule>(kTestCases);
}

// Tests that multi-line conditions don't require begin on same line
TEST(BeginSameLineRuleTest, AcceptsMultiLineConditions) {
  const std::initializer_list<LintTestCase> kTestCases = {
      // Multi-line if condition - begin can be on next line
      {"if (condition1 &&\n"
       "    condition2 &&\n"
       "    condition3)\n"
       "  begin\n"
       "    a <= 1;\n"
       "  end"},
      
      // Multi-line for condition
      {"for (int i = 0;\n"
       "     i < 10;\n"
       "     i++)\n"
       "  begin\n"
       "    a <= i;\n"
       "  end"},
      
      // Multi-line while condition
      {"while (a < 3 &&\n"
       "       b > 5)\n"
       "  begin\n"
       "    a = a + 1;\n"
       "  end"},
      
      // Multi-line foreach condition
      {"foreach (array[i,\n"
       "               j])\n"
       "  begin\n"
       "    sum += array[i][j];\n"
       "  end"},
      
      // Multi-line else-if condition
      {"else if (cond1 ||\n"
       "         cond2)\n"
       "  begin\n"
       "    a <= 1;\n"
       "  end"},
      
      // Multi-line always @ condition
      {"always @ (posedge clk or\n"
       "          negedge rst_n)\n"
       "  begin\n"
       "    if (rst_n) a <= 0;\n"
       "  end"},
  };
  RunLintTestCases<VerilogAnalyzer, BeginSameLineRule>(kTestCases);
}

// Tests that single-line conditions still require begin on same line
TEST(BeginSameLineRuleTest, SingleLineConditionStillRequiresSameLine) {
  const std::initializer_list<LintTestCase> kTestCases = {
      // Single-line condition, begin on next line - should fail
      {{TK_if, "if"}, " (condition)\n begin a <= 1; end"},
      {{TK_for, "for"}, " (i = 0; i < 10; i++)\n begin a <= 1; end"},
      {{TK_while, "while"}, " (a < 3)\n begin a <= 1; end"},
  };
  RunLintTestCases<VerilogAnalyzer, BeginSameLineRule>(kTestCases);
}

}  // namespace
}  // namespace analysis
}  // namespace verilog
