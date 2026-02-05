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

#include "verible/verilog/analysis/checkers/one-port-per-line-rule.h"

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

// Tests that OnePortPerLineRule correctly accepts single port per line.
TEST(OnePortPerLineRuleTest, AcceptTests) {
  const std::initializer_list<LintTestCase> kTestCases = {
      // Empty module
      {"module t; endmodule;"},

      // Single port per line
      {"module t (input a); endmodule;"},
      {"module t (input a,\n"
       "input b);\n"
       "endmodule;"},
      {"module t (input a,\n"
       "output b,\n"
       "inout c);\n"
       "endmodule;"},
      {"module t (input wire a,\n"
       "output reg b,\n"
       "inout wire c);\n"
       "endmodule;"},
      {"module t (\n"
       "input a,\n"
       "output b,\n"
       "inout c\n"
       ");\n"
       "endmodule;"},

      // Multiple identifiers in one port declaration but on separate lines
      {"module t (input a,\n"
       "b);\n"
       "endmodule;"},
      {"module t (input a,\n"
       "b,\n"
       "c);\n"
       "endmodule;"},
  };
  RunLintTestCases<VerilogAnalyzer, OnePortPerLineRule>(kTestCases);
}

// Tests that OnePortPerLineRule rejects multiple ports on same line.
TEST(OnePortPerLineRuleTest, RejectTests) {
  constexpr int kToken = SymbolIdentifier;
  const std::initializer_list<LintTestCase> kTestCases = {
      // Two identifiers in same port declaration on same line
      // 'b' is the second port on the line, so it's flagged
      {"module t (input a, ", {kToken, "b"}, "); endmodule;"},

      // Three identifiers - 'b' and 'c' are flagged
      {"module t (input a, ",
       {kToken, "b"},
       ", ",
       {kToken, "c"},
       "); endmodule;"},

      // Multiple port declarations on same line
      {"module t (input a, input ", {kToken, "b"}, "); endmodule;"},
      {"module t (input a, input ",
       {kToken, "b"},
       ", input ",
       {kToken, "c"},
       "); endmodule;"},
  };
  RunLintTestCases<VerilogAnalyzer, OnePortPerLineRule>(kTestCases);
}

}  // namespace
}  // namespace analysis
}  // namespace verilog
