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

#include "verible/verilog/analysis/checkers/ip-instantiation-format-rule.h"

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

// Tests that consistent port connection formatting passes
TEST(IPInstantiationFormatRuleTest, AcceptsConsistentFormat) {
  const std::initializer_list<LintTestCase> kTestCases = {
      // Single instance
      {"module m;\n"
       "  ip_module inst1 (\n"
       "    .clk(clk),\n"
       "    .rst(rst)\n"
       "  );\n"
       "endmodule"},
      // Multiple instances with consistent formatting (no space before paren)
      {"module m;\n"
       "  ip_module inst1 (\n"
       "    .clk(clk),\n"
       "    .rst(rst)\n"
       "  );\n"
       "  ip_module inst2 (\n"
       "    .clk(clk),\n"
       "    .rst(rst)\n"
       "  );\n"
       "endmodule"},
      // Multiple instances with consistent formatting (with space before paren)
      {"module m;\n"
       "  ip_module inst1 (\n"
       "    .clk (clk),\n"
       "    .rst (rst)\n"
       "  );\n"
       "  ip_module inst2 (\n"
       "    .clk (clk),\n"
       "    .rst (rst)\n"
       "  );\n"
       "endmodule"},
  };
  RunLintTestCases<VerilogAnalyzer, IPInstantiationFormatRule>(kTestCases);
}

// Tests that inconsistent port connection formatting is rejected
TEST(IPInstantiationFormatRuleTest, RejectsInconsistentFormat) {
  constexpr int kToken = TK_OTHER;
  const std::initializer_list<LintTestCase> kTestCases = {
      // Inconsistent left paren position within first instance
      {"module m;\n"
       "  ip_module inst1 (\n"
       "    .clk(clk),\n"
       "    ", {kToken, ".rst  (rst)"}, "\n"
       "  );\n"
       "endmodule"},
      // Inconsistent right paren position within first instance
      {"module m;\n"
       "  ip_module inst1 (\n"
       "    .clk(clk),\n"
       "    ", {kToken, ".data(data_signal)"}, "\n"
       "  );\n"
       "endmodule"},
      // Multiple instances where second instance doesn't match first instance format
      {"module m;\n"
       "  ip_module inst1 (\n"
       "    .clk(clk),\n"
       "    .rst(rst)\n"
       "  );\n"
       "  ip_module inst2 (\n"
       "    ", {kToken, ".clk (clk2)"}, ",\n"
       "    ", {kToken, ".rst (rst2)"}, "\n"
       "  );\n"
       "endmodule"},
  };
  RunLintTestCases<VerilogAnalyzer, IPInstantiationFormatRule>(kTestCases);
}

// Tests that non-instantiation code is ignored
TEST(IPInstantiationFormatRuleTest, IgnoresNonInstantiations) {
  const std::initializer_list<LintTestCase> kTestCases = {
      // Regular function calls
      {"module m;\n"
       "  initial begin\n"
       "    $display(\"test\");\n"
       "  end\n"
       "endmodule"},
      // Assignments
      {"module m;\n"
       "  assign out = in;\n"
       "endmodule"},
  };
  RunLintTestCases<VerilogAnalyzer, IPInstantiationFormatRule>(kTestCases);
}

// Tests that incorrect indentation is rejected
TEST(IPInstantiationFormatRuleTest, RejectsIncorrectIndentation) {
  constexpr int kToken = TK_OTHER;
  const std::initializer_list<LintTestCase> kTestCases = {
      // 2 spaces indentation on all ports (should be 4)
      {"module m;\n"
       "  ip_module inst1 (\n"
       "  ", {kToken, ".clk(clk)"}, ",\n"
       "  ", {kToken, ".rst(rst)"}, "\n"
       "  );\n"
       "endmodule"},
      // 6 spaces indentation on all ports (should be 4)
      {"module m;\n"
       "  ip_module inst1 (\n"
       "      ", {kToken, ".clk(clk)"}, ",\n"
       "      ", {kToken, ".rst(rst)"}, "\n"
       "  );\n"
       "endmodule"},
      // No indentation on all ports (should be 4 spaces)
      {"module m;\n"
       "  ip_module inst1 (\n"
       "", {kToken, ".clk(clk)"}, ",\n"
       "", {kToken, ".rst(rst)"}, "\n"
       "  );\n"
       "endmodule"},
  };
  RunLintTestCases<VerilogAnalyzer, IPInstantiationFormatRule>(kTestCases);
}

}  // namespace
}  // namespace analysis
}  // namespace verilog
