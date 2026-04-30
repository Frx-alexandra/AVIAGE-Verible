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

#include "verible/verilog/analysis/checkers/signal-initialization-rule.h"

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

TEST(SignalInitializationRuleTest, ValidInitialization) {
  const std::initializer_list<LintTestCase> kTestCases = {
      // Valid: All signals initialized in reset branch
      {"module m;\n"
       "always @(posedge clk) begin\n"
       "  if (rst) begin\n"
       "    q <= 1'b0;\n"
       "    r <= 1'b1;\n"
       "  end else begin\n"
       "    q <= d;\n"
       "    r <= e;\n"
       "  end\n"
       "end\n"
       "endmodule"},

      // Valid: Single signal initialized
      {"module m;\n"
       "always @(posedge clk) begin\n"
       "  if (reset) q <= 1'b0;\n"
       "  else q <= d;\n"
       "end\n"
       "endmodule"},

      // Valid: Initialization with parameter
      {"module m;\n"
       "always @(posedge clk) begin\n"
       "  if (rst_n) q <= INIT_VALUE;\n"
       "  else q <= data;\n"
       "end\n"
       "endmodule"},

      // Valid: Initialization with macro
      {"module m;\n"
       "always @(posedge clk) begin\n"
       "  if (i_rst) q <= `RESET_VAL;\n"
       "  else q <= d;\n"
       "end\n"
       "endmodule"},

      // Valid: Multiple signals initialized with different constants
      {"module m;\n"
       "always @(posedge clk) begin\n"
       "  if (reset) begin\n"
       "    counter <= 8'h00;\n"
       "    state <= 2'b00;\n"
       "    flag <= 1'b0;\n"
       "  end else begin\n"
       "    counter <= counter + 1;\n"
       "    state <= next_state;\n"
       "    flag <= enable;\n"
       "  end\n"
       "end\n"
       "endmodule"},

      // Valid: Negated reset signal
      {"module m;\n"
       "always @(posedge clk) begin\n"
       "  if (!rst_n) q <= 1'b0;\n"
       "  else q <= d;\n"
       "end\n"
       "endmodule"},

      // Valid: Complex reset expression
      {"module m;\n"
       "always @(posedge clk) begin\n"
       "  if (rst || soft_reset) q <= 1'b0;\n"
       "  else q <= d;\n"
       "end\n"
       "endmodule"},
  };

  RunLintTestCases<VerilogAnalyzer, SignalInitializationRule>(kTestCases);
}

TEST(SignalInitializationRuleTest, MissingInitialization) {
  constexpr int kToken = SymbolIdentifier;
  const std::initializer_list<LintTestCase> kTestCases = {
      // VIOLATION: Signal 'q' assigned but not initialized
      {"module m;\n"
       "always @(posedge clk) begin\n"
       "  if (rst) begin\n"
       "    // Missing initialization\n"
       "  end else begin\n"
       "    ",
       {kToken, "q"},
       " <= d;\n"
       "  end\n"
       "end\n"
       "endmodule"},

      // VIOLATION: Only one of two signals initialized
      {"module m;\n"
       "always @(posedge clk) begin\n"
       "  if (reset) begin\n"
       "    q <= 1'b0;\n"
       "    // r is missing initialization\n"
       "  end else begin\n"
       "    q <= d;\n"
       "    ",
       {kToken, "r"},
       " <= e;\n"
       "  end\n"
       "end\n"
       "endmodule"},

      // VIOLATION: Signal initialized outside reset branch
      {"module m;\n"
       "always @(posedge clk) begin\n"
       "  if (rst) begin\n"
       "    q <= 1'b0;\n"
       "  end else begin\n"
       "    ",
       {kToken, "r"},
       " <= 1'b0;\n"
       "  end\n"
       "  r <= data;  // Assignment outside if-else\n"
       "end\n"
       "endmodule"},

      // VIOLATION: Multiple signals missing initialization
      {"module m;\n"
       "always @(posedge clk) begin\n"
       "  if (rst_n) begin\n"
       "    // All missing initialization\n"
       "  end else begin\n"
       "    ",
       {kToken, "counter"},
       " <= counter + 1;\n"
       "    ",
       {kToken, "state"},
       " <= next_state;\n"
       "  end\n"
       "end\n"
       "endmodule"},

      // VIOLATION: Signal initialized with itself (self-reference)
      {"module m;\n"
       "always @(posedge clk) begin\n"
       "  if (rst) begin\n"
       "    ",
       {kToken, "counter"},
       " <= counter + 1;\n"
       "  end else begin\n"
       "    counter <= counter + 1;\n"
       "  end\n"
       "end\n"
       "endmodule"},
  };

  RunLintTestCases<VerilogAnalyzer, SignalInitializationRule>(kTestCases);
}

TEST(SignalInitializationRuleTest, NoResetCheck) {
  const std::initializer_list<LintTestCase> kTestCases = {
      // Not checked: No reset signal in first if
      {"module m;\n"
       "always @(posedge clk) begin\n"
       "  if (enable) q <= d;\n"
       "  else q <= 1'b0;\n"
       "end\n"
       "endmodule"},

      // Not checked: No if statement at all
      {"module m;\n"
       "always @(posedge clk) begin\n"
       "  q <= d;\n"
       "end\n"
       "endmodule"},

      // Not checked: Always block without reset
      {"module m;\n"
       "always @(posedge clk) begin\n"
       "  counter <= counter + 1;\n"
       "end\n"
       "endmodule"},
  };

  RunLintTestCases<VerilogAnalyzer, SignalInitializationRule>(kTestCases);
}

TEST(SignalInitializationRuleTest, DifferentAlwaysTypes) {
  constexpr int kToken = SymbolIdentifier;
  const std::initializer_list<LintTestCase> kTestCases = {
      // VIOLATION: always_ff block with missing initialization
      {"module m;\n"
       "always_ff @(posedge clk) begin\n"
       "  if (rst) begin\n"
       "    q <= 1'b0;\n"
       "  end else begin\n"
       "    q <= d;\n"
       "    ",
       {kToken, "r"},
       " <= e;\n"
       "  end\n"
       "end\n"
       "endmodule"},

      // Valid: always block with proper initialization
      {"module m;\n"
       "always @(posedge clk) begin\n"
       "  if (rst) begin\n"
       "    q <= 1'b0;\n"
       "    r <= 1'b1;\n"
       "  end else begin\n"
       "    q <= d;\n"
       "    r <= e;\n"
       "  end\n"
       "end\n"
       "endmodule"},
  };

  RunLintTestCases<VerilogAnalyzer, SignalInitializationRule>(kTestCases);
}

TEST(SignalInitializationRuleTest, ComplexCases) {
  constexpr int kToken = SymbolIdentifier;
  const std::initializer_list<LintTestCase> kTestCases = {
      // Valid: Nested begin-end blocks with initialization
      {"module m;\n"
       "always @(posedge clk) begin\n"
       "  if (rst) begin\n"
       "    q <= 1'b0;\n"
       "    r <= 1'b0;\n"
       "  end else begin\n"
       "    if (enable) begin\n"
       "      q <= d;\n"
       "      r <= e;\n"
       "    end\n"
       "  end\n"
       "end\n"
       "endmodule"},

      // VIOLATION: Signal in nested block not initialized
      {"module m;\n"
       "always @(posedge clk) begin\n"
       "  if (reset) begin\n"
       "    q <= 1'b0;\n"
       "  end else begin\n"
       "    q <= d;\n"
       "    if (enable) begin\n"
       "      ",
       {kToken, "r"},
       " <= e;\n"
       "    end\n"
       "  end\n"
       "end\n"
       "endmodule"},

      // Valid: Array element initialization
      {"module m;\n"
       "always @(posedge clk) begin\n"
       "  if (rst) begin\n"
       "    mem[0] <= 8'h00;\n"
       "  end else begin\n"
       "    mem[0] <= data;\n"
       "  end\n"
       "end\n"
       "endmodule"},

      // Valid: Bit-select initialization
      {"module m;\n"
       "always @(posedge clk) begin\n"
       "  if (rst) begin\n"
       "    reg_val[7:0] <= 8'h00;\n"
       "  end else begin\n"
       "    reg_val[7:0] <= data;\n"
       "  end\n"
       "end\n"
       "endmodule"},
  };

  RunLintTestCases<VerilogAnalyzer, SignalInitializationRule>(kTestCases);
}

TEST(SignalInitializationRuleTest, EdgeCases) {
  const std::initializer_list<LintTestCase> kTestCases = {
      // Valid: Case-insensitive reset signal names
      {"module m;\n"
       "always @(posedge clk) begin\n"
       "  if (RST) q <= 1'b0;\n"
       "  else q <= d;\n"
       "end\n"
       "endmodule"},

      {"module m;\n"
       "always @(posedge clk) begin\n"
       "  if (RESET) q <= 1'b0;\n"
       "  else q <= d;\n"
       "end\n"
       "endmodule"},

      {"module m;\n"
       "always @(posedge clk) begin\n"
       "  if (SysReset) q <= 1'b0;\n"
       "  else q <= d;\n"
       "end\n"
       "endmodule"},

      // Valid: Reset signal with prefix/suffix
      {"module m;\n"
       "always @(posedge clk) begin\n"
       "  if (arst_n) q <= 1'b0;\n"
       "  else q <= d;\n"
       "end\n"
       "endmodule"},

      {"module m;\n"
       "always @(posedge clk) begin\n"
       "  if (reset_sync) q <= 1'b0;\n"
       "  else q <= d;\n"
       "end\n"
       "endmodule"},
  };

  RunLintTestCases<VerilogAnalyzer, SignalInitializationRule>(kTestCases);
}

TEST(SignalInitializationRuleTest, StrictInitializationValidation) {
  constexpr int kToken = SymbolIdentifier;
  const std::initializer_list<LintTestCase> kTestCases = {
      // VIOLATION: Initialization with signal reference (lowercase)
      {"module m;\n"
       "reg data;\n"
       "always @(posedge clk) begin\n"
       "  if (rst) begin\n"
       "    ",
       {kToken, "q"},
       " <= data;\n"
       "  end else begin\n"
       "    q <= 1'b0;\n"
       "  end\n"
       "end\n"
       "endmodule"},

      // VIOLATION: Initialization with wire signal (w_ prefix)
      {"module m;\n"
       "wire w_data;\n"
       "always @(posedge clk) begin\n"
       "  if (rst) begin\n"
       "    ",
       {kToken, "q"},
       " <= w_data;\n"
       "  end else begin\n"
       "    q <= 1'b0;\n"
       "  end\n"
       "end\n"
       "endmodule"},

      // VIOLATION: Initialization with reg signal (r_ prefix)
      {"module m;\n"
       "reg r_other;\n"
       "always @(posedge clk) begin\n"
       "  if (rst) begin\n"
       "    ",
       {kToken, "q"},
       " <= r_other;\n"
       "  end else begin\n"
       "    q <= 1'b0;\n"
       "  end\n"
       "end\n"
       "endmodule"},

      // VIOLATION: Expression with signal + constant
      {"module m;\n"
       "reg counter;\n"
       "always @(posedge clk) begin\n"
       "  if (rst) begin\n"
       "    ",
       {kToken, "q"},
       " <= counter + 8'h10;\n"
       "  end else begin\n"
       "    q <= 1'b0;\n"
       "  end\n"
       "end\n"
       "endmodule"},

      // Valid: Initialization with ALL_CAPS parameter
      {"module m;\n"
       "parameter RESET_VAL = 8'h00;\n"
       "always @(posedge clk) begin\n"
       "  if (rst) begin\n"
       "    q <= RESET_VAL;\n"
       "  end else begin\n"
       "    q <= 1'b0;\n"
       "  end\n"
       "end\n"
       "endmodule"},

      // Valid: Initialization with CamelCase parameter
      {"module m;\n"
       "localparam InitValue = 8'h00;\n"
       "always @(posedge clk) begin\n"
       "  if (rst) begin\n"
       "    q <= InitValue;\n"
       "  end else begin\n"
       "    q <= 1'b0;\n"
       "  end\n"
       "end\n"
       "endmodule"},

      // Valid: Parameter arithmetic
      {"module m;\n"
       "parameter BASE = 8'h10;\n"
       "parameter OFFSET = 8'h05;\n"
       "always @(posedge clk) begin\n"
       "  if (rst) begin\n"
       "    q <= BASE + OFFSET;\n"
       "  end else begin\n"
       "    q <= 1'b0;\n"
       "  end\n"
       "end\n"
       "endmodule"},

      // Valid: Mixed constant and parameter arithmetic
      {"module m;\n"
       "parameter BASE = 8'h10;\n"
       "always @(posedge clk) begin\n"
       "  if (rst) begin\n"
       "    q <= (BASE + 8'h10) << 2;\n"
       "  end else begin\n"
       "    q <= 1'b0;\n"
       "  end\n"
       "end\n"
       "endmodule"},
  };

  RunLintTestCases<VerilogAnalyzer, SignalInitializationRule>(kTestCases);
}

}  // namespace
}  // namespace analysis
}  // namespace verilog
