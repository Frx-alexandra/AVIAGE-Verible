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
#include "verible/common/analysis/text-structure-linter-test-utils.h"
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
  };
  RunLintTestCases<VerilogAnalyzer, OneStatementPerLineRule>(kTestCases);
}

// Tests that OneStatementPerLineRule rejects multiple statements on same line.
TEST(OneStatementPerLineRuleTest, RejectTests) {
  // The rule reports the first token of the violating statement
  // For assignments, this is the variable identifier (SymbolIdentifier)
  constexpr int kIdToken = SymbolIdentifier;
  const std::initializer_list<LintTestCase> kTestCases = {
      // Two procedural assignments on same line - second 'b' is violation
      {"module t; always @(*) begin a = 1; ",
       {kIdToken, "b"},
       " = 2; end endmodule;"},

      // Multiple continuous assignments on same line
      // The variable 'b' in the second assignment is the violation
      {"module t; assign a = 1; assign ", {kIdToken, "b"}, " = 2; endmodule;"},

      // Three statements on same line - second and third are violations
      {"module t; always @(*) begin a = 1; ",
       {kIdToken, "b"},
       " = 2; ",
       {kIdToken, "c"},
       " = 3; end endmodule;"},
  };
  RunLintTestCases<VerilogAnalyzer, OneStatementPerLineRule>(kTestCases);
}

}  // namespace
}  // namespace analysis
}  // namespace verilog
