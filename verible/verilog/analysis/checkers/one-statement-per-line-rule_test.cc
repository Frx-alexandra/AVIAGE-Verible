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

// Tests that OneStatementPerLineRule correctly accepts single statement per
// line.
TEST(OneStatementPerLineRuleTest, AcceptTests) {
  const std::initializer_list<LintTestCase> kTestCases = {
      // Empty module
      {"module t; endmodule;"},

      // Single statement
      {"module t; a = 1; endmodule;"},

      // Single assignment
      {"module t; assign a = 1; endmodule;"},

      // Single if with begin/end - statement on new line after begin
      {"if (cond)\n a = 1;"},

      // Single case item on new line after label
      {"case (x)\n 1:\n a = 1;\n endcase"},

      // Single always block with statement on new line
      {"always @(*)\n begin\n a = 1;\n end"},

      // Single for loop with statement on new line
      {"for (i=0; i<10; i++)\n begin\n a = i;\n end"},

      // Single while loop with statement on new line
      {"while (cond)\n begin\n a = 1;\n end"},

      // Single initial block with statement on new line
      {"initial\n begin\n a = 1;\n end"},

      // Single begin block with statement on new line
      {"begin\n a = 1;\n end"},
  };
  RunLintTestCases<VerilogAnalyzer, OneStatementPerLineRule>(kTestCases);
}

// Tests that OneStatementPerLineRule rejects statements not starting on new
// line.
TEST(OneStatementPerLineRuleTest, RejectTests) {
  constexpr int kIdToken = SymbolIdentifier;
  const std::initializer_list<LintTestCase> kTestCases = {
      // Test 1: Statement after if condition on same line
      {"if (cond) ", {kIdToken, "a"}, " = 1;"},

      // Test 2: Statement after begin on same line
      {"if (cond) begin ", {kIdToken, "a"}, " = 1; end"},

      // Test 3: Statement after case label on same line
      {"case (x) 1: ", {kIdToken, "a"}, " = 1; endcase"},

      // Test 4: Statement after begin in always block on same line
      {"always @(*) begin ", {kIdToken, "a"}, " = 1; end"},

      // Test 5: Statement after begin in for loop on same line
      {"for (i=0; i<10; i++) begin ", {kIdToken, "a"}, " = i; end"},

      // Test 6: Statement after begin in while loop on same line
      {"while (cond) begin ", {kIdToken, "a"}, " = 1; end"},

      // Test 7: Statement after begin in initial block on same line
      {"initial begin ", {kIdToken, "a"}, " = 1; end"},

      // Test 8: Statement after begin on same line
      {"begin ", {kIdToken, "a"}, " = 1; end"},

      // Test 9: Multiple statements - only first violation reported
      {"if (cond) begin ", {kIdToken, "a"}, " = 1; b = 2; end"},

      // Test 10: Second assign statement on same line
      {"assign a = 1; assign ", {kIdToken, "b"}, " = 2;"},
  };
  RunLintTestCases<VerilogAnalyzer, OneStatementPerLineRule>(kTestCases);
}

}  // namespace
}  // namespace analysis
}  // namespace verilog
