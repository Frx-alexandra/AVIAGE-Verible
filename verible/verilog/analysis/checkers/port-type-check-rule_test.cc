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

#include <initializer_list>

#include "gtest/gtest.h"
#include "verible/common/analysis/linter-test-utils.h"
#include "verible/common/analysis/syntax-tree-linter-test-utils.h"
#include "verible/verilog/analysis/verilog-analyzer.h"
#include "verible/verilog/parser/verilog-token-enum.h"

namespace verilog {
namespace analysis {
namespace {

using verible::LintTestCase;
using verible::RunLintTestCases;

// Tests that PortTypeCheckRule correctly accepts valid port types.
TEST(PortTypeCheckRuleTest, AcceptTests) {
  const std::initializer_list<LintTestCase> kTestCases = {
      // Empty module
      {"module t; endmodule;"},

      // Input ports with wire type (valid)
      {"module t (input wire a); endmodule;"},
      {"module t (input wire [7:0] data); endmodule;"},

      // Output ports with wire type (valid)
      {"module t (output wire b); endmodule;"},
      {"module t (output wire [3:0] result); endmodule;"},

      // Output ports with reg type (valid)
      {"module t (output reg c); endmodule;"},
      {"module t (output reg [7:0] data_out); endmodule;"},

      // Inout ports with wire type (valid)
      {"module t (inout wire d); endmodule;"},
      {"module t (inout wire [15:0] bus); endmodule;"},

      // Multiple valid ports
      {"module t (input wire a, output wire b, output reg c, inout wire d); "
       "endmodule;"},
      {"module t (input wire [7:0] data_i, "
       "output reg [7:0] data_o, "
       "inout wire [7:0] bus_io); "
       "endmodule;"},

      // Implicit type (defaults to wire, which is valid)
      {"module t (input a); endmodule;"},
      {"module t (output b); endmodule;"},
      {"module t (inout c); endmodule;"},
      {"module t (input [7:0] data); endmodule;"},
      {"module t (input a, output b, inout c); endmodule;"},
  };
  RunLintTestCases<VerilogAnalyzer, PortTypeCheckRule>(kTestCases);
}

// Tests that PortTypeCheckRule rejects invalid port types.
TEST(PortTypeCheckRuleTest, RejectTests) {
  constexpr int kToken = SymbolIdentifier;
  const std::initializer_list<LintTestCase> kTestCases = {
      // Input ports with invalid types
      {"module t (input reg ", {kToken, "a"}, "); endmodule;"},
      {"module t (input logic ", {kToken, "b"}, "); endmodule;"},
      {"module t (input reg [7:0] ", {kToken, "data"}, "); endmodule;"},

      // Output ports with invalid types (logic is invalid)
      {"module t (output logic ", {kToken, "c"}, "); endmodule;"},
      {"module t (output logic [3:0] ", {kToken, "result"}, "); endmodule;"},

      // Inout ports with invalid types
      {"module t (inout reg ", {kToken, "d"}, "); endmodule;"},
      {"module t (inout logic ", {kToken, "e"}, "); endmodule;"},
      {"module t (inout reg [15:0] ", {kToken, "bus"}, "); endmodule;"},

      // Multiple invalid ports
      {"module t (input reg ",
       {kToken, "a"},
       ", output logic ",
       {kToken, "b"},
       ", inout reg ",
       {kToken, "c"},
       "); endmodule;"},
  };
  RunLintTestCases<VerilogAnalyzer, PortTypeCheckRule>(kTestCases);
}

}  // namespace
}  // namespace analysis
}  // namespace verilog
