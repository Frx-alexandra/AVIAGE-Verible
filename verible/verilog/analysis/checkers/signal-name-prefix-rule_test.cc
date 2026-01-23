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

#include "verible/verilog/analysis/checkers/signal-name-prefix-rule.h"

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

// Tests that SignalNamePrefixRule correctly accepts valid names.
TEST(SignalNamePrefixRuleTest, AcceptTests) {
  const std::initializer_list<LintTestCase> kTestCases = {
      {""},
      {"module t; input logic i_name; output logic o_name; inout logic "
       "io_name; endmodule;"},
      {"module t; input logic [7:0] i_data; output logic [2:0] o_ctrl; inout "
       "logic [3:0] io_bus; endmodule;"},
      {"module t; wire w_signal; reg r_register; endmodule;"},
      {"module t; wire w_data; reg r_state; endmodule;"},
      {"module t; input logic i_clk; input logic i_rst; output logic o_valid; "
       "endmodule;"},
      {"module t; wire w_ready; reg r_done; inout logic io_ack; endmodule;"},
      {"module t; input bit i_enable; output bit o_error; inout bit io_data; "
       "endmodule;"},
      {"module t; wire w_int; reg r_flag; endmodule;"},
      {"module t; input logic i_long_name; output logic o_long_name; inout "
       "logic io_long_name; endmodule;"},
      {"module t; wire w_long_signal_name; reg r_long_register_name; "
       "endmodule;"},
      {"module t; tri t_signal; wor w_or; triand ta_signal; trior to_signal; "
       "endmodule;"},
      {"module t; input logic i_name,\n"
       "output logic o_name,\n"
       "inout logic io_name,\n"
       "wire w_signal,\n"
       "reg r_register,\n"
       "input logic [7:0] i_data,\n"
       "output logic [2:0] o_ctrl,\n"
       "inout logic [3:0] io_bus,\n"
       "wire [15:0] w_wide,\n"
       "reg [31:0] r_wide);\n"
       "endmodule;"},
  };
  RunLintTestCases<VerilogAnalyzer, SignalNamePrefixRule>(kTestCases);
}

// Tests that SignalNamePrefixRule rejects invalid names.
TEST(SignalNamePrefixRuleTest, RejectTests) {
  constexpr int kToken = SymbolIdentifier;
  const std::initializer_list<LintTestCase> kTestCases = {
      // Input port tests
      {"module t; input logic ", {kToken, "name"}, "; endmodule;"},
      {"module t; input logic ", {kToken, "o_name"}, "; endmodule;"},
      {"module t; input logic ", {kToken, "r_name"}, "; endmodule;"},
      {"module t; input logic ", {kToken, "w_name"}, "; endmodule;"},
      {"module t; input logic ", {kToken, "io_name"}, "; endmodule;"},
      {"module t; input logic ", {kToken, "namei"}, "; endmodule;"},
      {"module t; input logic ", {kToken, "name_i"}, "; endmodule;"},

      // Output port tests
      {"module t; output logic ", {kToken, "name"}, "; endmodule;"},
      {"module t; output logic ", {kToken, "i_name"}, "; endmodule;"},
      {"module t; output logic ", {kToken, "r_name"}, "; endmodule;"},
      {"module t; output logic ", {kToken, "w_name"}, "; endmodule;"},
      {"module t; output logic ", {kToken, "io_name"}, "; endmodule;"},
      {"module t; output logic ", {kToken, "nameo"}, "; endmodule;"},
      {"module t; output logic ", {kToken, "name_o"}, "; endmodule;"},

      // Inout port tests
      {"module t; inout logic ", {kToken, "name"}, "; endmodule;"},
      {"module t; inout logic ", {kToken, "i_name"}, "; endmodule;"},
      {"module t; inout logic ", {kToken, "o_name"}, "; endmodule;"},
      {"module t; inout logic ", {kToken, "r_name"}, "; endmodule;"},
      {"module t; inout logic ", {kToken, "w_name"}, "; endmodule;"},
      {"module t; inout logic ", {kToken, "nameio"}, "; endmodule;"},
      {"module t; inout logic ", {kToken, "name_io"}, "; endmodule;"},

      // Wire tests
      {"module t; wire ", {kToken, "name"}, "; endmodule;"},
      {"module t; wire ", {kToken, "i_name"}, "; endmodule;"},
      {"module t; wire ", {kToken, "o_name"}, "; endmodule;"},
      {"module t; wire ", {kToken, "io_name"}, "; endmodule;"},
      {"module t; wire ", {kToken, "r_name"}, "; endmodule;"},
      {"module t; wire ", {kToken, "namew"}, "; endmodule;"},
      {"module t; wire ", {kToken, "name_w"}, "; endmodule;"},

      // Reg tests
      {"module t; reg ", {kToken, "name"}, "; endmodule;"},
      {"module t; reg ", {kToken, "i_name"}, "; endmodule;"},
      {"module t; reg ", {kToken, "o_name"}, "; endmodule;"},
      {"module t; reg ", {kToken, "io_name"}, "; endmodule;"},
      {"module t; reg ", {kToken, "w_name"}, "; endmodule;"},
      {"module t; reg ", {kToken, "namer"}, "; endmodule;"},
      {"module t; reg ", {kToken, "name_r"}, "; endmodule;"},

      // Invalid casing
      {"module t; input logic ", {kToken, "I_name"}, "; endmodule;"},
      {"module t; output logic ", {kToken, "O_name"}, "; endmodule;"},
      {"module t; inout logic ", {kToken, "IO_name"}, "; endmodule;"},
      {"module t; wire ", {kToken, "W_name"}, "; endmodule;"},
      {"module t; reg ", {kToken, "R_name"}, "; endmodule;"},

      // No underscore
      {"module t; input logic ", {kToken, "iname"}, "; endmodule;"},
      {"module t; output logic ", {kToken, "oname"}, "; endmodule;"},
      {"module t; inout logic ", {kToken, "ioname"}, "; endmodule;"},
      {"module t; wire ", {kToken, "wname"}, "; endmodule;"},
      {"module t; reg ", {kToken, "rname"}, "; endmodule;"},

  };
  RunLintTestCases<VerilogAnalyzer, SignalNamePrefixRule>(kTestCases);
}

}  // namespace
}  // namespace analysis
}  // namespace verilog