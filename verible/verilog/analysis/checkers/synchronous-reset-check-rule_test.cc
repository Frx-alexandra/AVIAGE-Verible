// Copyright 2017-2023 The Verible Authors.
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

#include "verible/verilog/analysis/checkers/synchronous-reset-check-rule.h"

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

TEST(SynchronousResetCheckTest, ValidSynchronousReset) {
  const std::initializer_list<LintTestCase> kTestCases = {
      // Valid: Synchronous reset with i_clk (contains "clk")
      {"module m;\n"
       "always @(posedge i_clk) begin\n"
       "  if (i_rst) q <= 1'b0;\n"
       "  else q <= d;\n"
       "end\n"
       "endmodule"},

      // Valid: Synchronous reset with sys_clock (contains "clock")
      {"module m;\n"
       "always @(negedge sys_clock) begin\n"
       "  if (system_reset) q <= 1'b0;\n"
       "  else q <= d;\n"
       "end\n"
       "endmodule"},

      // Valid: Reset with rst_n (contains "rst")
      {"module m;\n"
       "always @(posedge my_clk) begin\n"
       "  if (!rst_n) q <= 1'b0;\n"
       "  else q <= d;\n"
       "end\n"
       "endmodule"},

      // Valid: Reset with reset keyword
      {"module m;\n"
       "always @(posedge refclk) begin\n"
       "  if (async_reset) q <= RESET_VAL;\n"
       "  else if (enable) q <= data;\n"
       "end\n"
       "endmodule"},

      // Valid: Case-insensitive clock and reset
      {"module m;\n"
       "always @(posedge CLK) begin\n"
       "  if (RST) q <= 1'b0;\n"
       "  else q <= d;\n"
       "end\n"
       "endmodule"},

      // Valid: Different clock/reset combinations
      {"module m;\n"
       "always @(posedge clock_i) begin\n"
       "  if (rst_i) q <= 1'b0;\n"
       "  else q <= d;\n"
       "end\n"
       "endmodule"},
  };

  RunLintTestCases<VerilogAnalyzer, SynchronousResetCheckRule>(kTestCases);
}

TEST(SynchronousResetCheckTest, InvalidAsyncReset) {
  constexpr int kToken = TK_always;
  const std::initializer_list<LintTestCase> kTestCases = {
      // VIOLATION: Async reset in sensitivity list (multiple signals)
      {"module m;\n",
       {kToken, "always"},
       " @(posedge i_clk or posedge i_rst) begin\n"
       "  if (i_rst) q <= 1'b0;\n"
       "  else q <= d;\n"
       "end\n"
       "endmodule"},

      // VIOLATION: Async reset with negedge
      {"module m;\n",
       {kToken, "always"},
       " @(posedge i_clk or negedge rst_n) begin\n"
       "  if (!rst_n) q <= 1'b0;\n"
       "  else q <= d;\n"
       "end\n"
       "endmodule"},

      // VIOLATION: Async reset with comma syntax
      {"module m;\n",
       {kToken, "always"},
       " @(posedge i_clk, posedge reset) begin\n"
       "  if (reset) q <= 1'b0;\n"
       "  else q <= d;\n"
       "end\n"
       "endmodule"},

      // VIOLATION: Multiple non-reset signals in sensitivity
      {"module m;\n",
       {kToken, "always"},
       " @(posedge i_clk or posedge set) begin\n"
       "  if (set) q <= 1'b1;\n"
       "  else q <= d;\n"
       "end\n"
       "endmodule"},
  };

  RunLintTestCases<VerilogAnalyzer, SynchronousResetCheckRule>(kTestCases);
}

TEST(SynchronousResetCheckTest, InvalidNoReset) {
  constexpr int kToken = TK_always;
  const std::initializer_list<LintTestCase> kTestCases = {
      // VIOLATION: No reset check inside block
      {"module m;\n",
       {kToken, "always"},
       " @(posedge i_clk) begin\n"
       "  q <= d;\n"
       "end\n"
       "endmodule"},

      // VIOLATION: Check non-reset signal
      {"module m;\n",
       {kToken, "always"},
       " @(posedge i_clk) begin\n"
       "  if (enable) q <= d;\n"
       "  else q <= 1'b0;\n"
       "end\n"
       "endmodule"},

      // VIOLATION: No reset check (flag is not a reset signal)
      {"module m;\n",
       {kToken, "always"},
       " @(posedge i_clk) begin\n"
       "  if (flag) q <= 1'b0;\n"
       "  else q <= d;\n"
       "end\n"
       "endmodule"},
  };

  RunLintTestCases<VerilogAnalyzer, SynchronousResetCheckRule>(kTestCases);
}

TEST(SynchronousResetCheckTest, InvalidNoClock) {
  constexpr int kToken = TK_always;
  const std::initializer_list<LintTestCase> kTestCases = {
      // VIOLATION: No clock-pattern signal in sensitivity
      {"module m;\n",
       {kToken, "always"},
       " @(posedge enable) begin\n"
       "  if (i_rst) q <= 1'b0;\n"
       "  else q <= d;\n"
       "end\n"
       "endmodule"},

      // VIOLATION: data_valid is not a clock signal
      {"module m;\n",
       {kToken, "always"},
       " @(posedge data_valid) begin\n"
       "  if (reset) q <= 1'b0;\n"
       "  else q <= d;\n"
       "end\n"
       "endmodule"},
  };

  RunLintTestCases<VerilogAnalyzer, SynchronousResetCheckRule>(kTestCases);
}

TEST(SynchronousResetCheckTest, IgnoredBlocks) {
  const std::initializer_list<LintTestCase> kTestCases = {
      // Not checked: always_comb blocks
      {"module m;\n"
       "always_comb begin a = b; end\n"
       "endmodule"},

      // Not checked: always_latch blocks
      {"module m;\n"
       "always_latch begin a <= b; end\n"
       "endmodule"},

      // Not checked: always_ff blocks
      {"module m;\n"
       "always_ff @(posedge i_clk) q <= d;\n"
       "endmodule"},
  };

  RunLintTestCases<VerilogAnalyzer, SynchronousResetCheckRule>(kTestCases);
}

TEST(SynchronousResetCheckTest, EdgeCases) {
  constexpr int kToken = TK_always;
  const std::initializer_list<LintTestCase> kTestCases = {
      // VIOLATION: Both no clock and no reset
      {"module m;\n",
       {kToken, "always"},
       " @(posedge data) begin\n"
       "  q <= d;\n"
       "end\n"
       "endmodule"},

      // Valid: Multiple statements in reset check
      {"module m;\n"
       "always @(posedge i_clk) begin\n"
       "  if (i_rst) begin\n"
       "    q <= 1'b0;\n"
       "    r <= 1'b1;\n"
       "  end else begin\n"
       "    q <= d;\n"
       "    r <= e;\n"
       "  end\n"
       "end\n"
       "endmodule"},

      // VIOLATION: Multiple always blocks - one valid, one invalid
      {"module m;\n"
       "always @(posedge i_clk) begin\n"
       "  if (i_rst) q1 <= 1'b0;\n"
       "  else q1 <= d1;\n"
       "end\n"
       "",
       {kToken, "always"},
       " @(posedge i_clk or posedge i_rst) begin\n"
       "  if (i_rst) q2 <= 1'b0;\n"
       "  else q2 <= d2;\n"
       "end\n"
       "endmodule"},
  };

  RunLintTestCases<VerilogAnalyzer, SynchronousResetCheckRule>(kTestCases);
}

}  // namespace
}  // namespace analysis
}  // namespace verilog
