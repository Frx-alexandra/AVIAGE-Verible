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

#include "verible/verilog/analysis/checkers/function-header-check-rule.h"

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

TEST(FunctionHeaderCheckTest, ValidFunctionHeader) {
  const std::initializer_list<LintTestCase> kTestCases = {
      // Valid: FUNCTION in comment before function
      {"// FUNCTION: my_function\n"
       "function int my_function();\n"
       "  return 0;\n"
       "endfunction"},

      // Valid: FUNCTION in block comment before function
      {"/* FUNCTION: my_function */\n"
       "function int my_function();\n"
       "  return 0;\n"
       "endfunction"},

      // Valid: FUNCTION in comment with extra text
      {"// This is a FUNCTION declaration\n"
       "function int my_function();\n"
       "  return 0;\n"
       "endfunction"},

      // Valid: Multiple functions with headers
      {"// FUNCTION: func1\n"
       "function int func1();\n"
       "  return 0;\n"
       "endfunction\n"
       "// FUNCTION: func2\n"
       "function int func2();\n"
       "  return 1;\n"
       "endfunction"},

      // Valid: No function - no violation
      {"module test;\n"
       "endmodule"},

      // Valid: Function in module with header
      {"module test;\n"
       "// FUNCTION: calc\n"
       "function int calc(input a);\n"
       "  return a;\n"
       "endfunction\n"
       "endmodule"},
  };

  RunLintTestCases<VerilogAnalyzer, FunctionHeaderCheckRule>(kTestCases);
}

TEST(FunctionHeaderCheckTest, InvalidFunctionHeader) {
  constexpr int kToken = TK_OTHER;
  const std::initializer_list<LintTestCase> kTestCases = {
      // Invalid: No FUNCTION comment before function
      {"",
       {kToken, "function"},
       " int my_function();\n"
       "  return 0;\n"
       "endfunction"},

      // Invalid: Wrong keyword in comment
      {"// This is a module\n"
       "",
       {kToken, "function"},
       " int my_function();\n"
       "  return 0;\n"
       "endfunction"},

      // Invalid: FUNCTION after function
      {"",
       {kToken, "function"},
       " int my_function();\n"
       "  return 0;\n"
       "endfunction\n"
       "// FUNCTION: my_function"},

      // Invalid: FUNCTION in non-comment text
      {"string s = \"This is a FUNCTION\";\n"
       "",
       {kToken, "function"},
       " int my_function();\n"
       "  return 0;\n"
       "endfunction"},

      // Invalid: Multiple functions, second one missing header
      {"// FUNCTION: func1\n"
       "function int func1();\n"
       "  return 0;\n"
       "endfunction\n"
       "",
       {kToken, "function"},
       " int func2();\n"
       "  return 1;\n"
       "endfunction"},
  };

  RunLintTestCases<VerilogAnalyzer, FunctionHeaderCheckRule>(kTestCases);
}

TEST(FunctionHeaderCheckTest, EdgeCases) {
  constexpr int kToken = TK_OTHER;
  const std::initializer_list<LintTestCase> kTestCases = {
      // Valid: FUNCTION with different case
      {"// function: my_func\n"
       "function int my_func();\n"
       "  return 0;\n"
       "endfunction"},

      // Valid: FUNCTION in inline comment
      {"function int func(); // FUNCTION\n"
       "  return 0;\n"
       "endfunction"},

      // Invalid: Empty file
      {""},

      // Valid: Function with only whitespace before
      {"   \n"
       "// FUNCTION: test\n"
       "function void test();\n"
       "endfunction"},

      // Valid: Function with blank lines before header
      {"\n"
       "\n"
       "// FUNCTION: test\n"
       "function void test();\n"
       "endfunction"},

      // Invalid: Function immediately after module declaration
      {"module test;\n"
       "",
       {kToken, "function"},
       " int calc();\n"
       "  return 0;\n"
       "endfunction\n"
       "endmodule"},
  };

  RunLintTestCases<VerilogAnalyzer, FunctionHeaderCheckRule>(kTestCases);
}

}  // namespace
}  // namespace analysis
}  // namespace verilog
