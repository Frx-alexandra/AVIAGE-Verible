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

#include "verible/verilog/analysis/checkers/wire-reg-declaration-alignment-rule.h"

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

// Tests that properly aligned declarations pass
TEST(WireRegDeclarationAlignmentRuleTest, AcceptsConsistentAlignment) {
  const std::initializer_list<LintTestCase> kTestCases = {
      // Single wire declaration
      {"module m; wire a; endmodule"},
      // Single reg declaration
      {"module m; reg b; endmodule"},
      // Multiple declarations with consistent alignment (same name length)
      {"module m;\n"
       "  wire  sig1;\n"
       "  wire  sig2;\n"
       "endmodule"},
      // Multiple declarations with ranges, consistent alignment
      {"module m;\n"
       "  wire [7:0] data_a;\n"
       "  reg  [7:0] data_b;\n"
       "endmodule"},
      // Consistent alignment requires same lengths
      {"module m;\n"
       "  wire [7:0] signal_a;\n"
       "  wire [7:0] signal_b;\n"
       "endmodule"},
  };
  RunLintTestCases<VerilogAnalyzer, WireRegDeclarationAlignmentRule>(kTestCases);
}

// Tests that inconsistent alignment is rejected
TEST(WireRegDeclarationAlignmentRuleTest, RejectsInconsistentAlignment) {
  constexpr int kToken = TK_OTHER;
  const std::initializer_list<LintTestCase> kTestCases = {
      // Inconsistent name start position
      {"module m;\n"
       "  wire  sig1;\n"
       "  wire ", {kToken, "sig2"}, ";\n"
       "endmodule"},
      // Inconsistent range start position
      {"module m;\n"
       "  wire [7:0] data_a;\n"
       "  wire ", {kToken, "[15:0] data_b"}, ";\n"
       "endmodule"},
      // Inconsistent semicolon position
      {"module m;\n"
       "  wire [7:0] data_aaa;\n"
       "  wire ", {kToken, "[7:0] data_b"}, ";\n"
       "endmodule"},
      // Multiple violations in single line
      {"module m;\n"
       "  wire [7:0]  signal_one;\n"
       "  wire ", {kToken, "[15:0] sig"}, ";\n"
       "endmodule"},
  };
  RunLintTestCases<VerilogAnalyzer, WireRegDeclarationAlignmentRule>(kTestCases);
}

// Tests mixed wire and reg declarations
TEST(WireRegDeclarationAlignmentRuleTest, MixedWireAndReg) {
  const std::initializer_list<LintTestCase> kTestCases = {
      // Properly aligned mix
      {"module m;\n"
       "  wire [7:0] data_w;\n"
       "  reg  [7:0] data_r;\n"
       "endmodule"},
  };
  RunLintTestCases<VerilogAnalyzer, WireRegDeclarationAlignmentRule>(kTestCases);
}

// Tests that non-declaration lines are ignored
TEST(WireRegDeclarationAlignmentRuleTest, IgnoresNonDeclarations) {
  const std::initializer_list<LintTestCase> kTestCases = {
      // Mixed with assignments
      {"module m;\n"
       "  wire [7:0] data_a;\n"
       "  assign data_a = 8'h00;\n"
       "  reg  [7:0] data_b;\n"
       "endmodule"},
      // With comments
      {"module m;\n"
       "  // This is a comment\n"
       "  wire [7:0] data_a;\n"
       "  /* block comment */\n"
       "  wire [7:0] data_b;\n"
       "endmodule"},
  };
  RunLintTestCases<VerilogAnalyzer, WireRegDeclarationAlignmentRule>(kTestCases);
}

}  // namespace
}  // namespace analysis
}  // namespace verilog
