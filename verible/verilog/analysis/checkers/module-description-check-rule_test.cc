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

#include "verible/verilog/analysis/checkers/module-description-check-rule.h"

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

// Tests that files with Module Name before module are accepted.
TEST(ModuleDescriptionCheckRuleTest, AcceptsText) {
  const std::initializer_list<LintTestCase> kTestCases = {
      // Single line comment with Module Name before module
      {"// Module Name: test_module\nmodule test_module; endmodule"},
      {"// Module Name: my_design\nmodule my_design; endmodule"},
      {"// Module Name: test_module - Test Module\nmodule test_module; "
       "endmodule"},
      // Block comment with Module Name before module
      {"/* Module Name: test_module */\nmodule test_module; endmodule"},
      {"/*\n * Module Name: test_module\n * Description: test\n */\nmodule "
       "test_module; endmodule"},
      // Multiple comments before module
      {"// Copyright 2024\n// Module Name: test_module\nmodule test_module; "
       "endmodule"},
      {"// Comment 1\n// Comment 2\n// Module Name: test_module\nmodule "
       "test_module; endmodule"},
      // Module Name with special characters
      {"// Module Name: my_module_v1.0\nmodule my_module_v1; endmodule"},
      {"// Module Name: test-module\nmodule test_module; endmodule"},
  };
  RunLintTestCases<VerilogAnalyzer, ModuleDescriptionCheckRule>(kTestCases);
}

// Tests that files without Module Name are rejected.
TEST(ModuleDescriptionCheckRuleTest, RejectsText) {
  constexpr int kToken = TK_OTHER;
  const std::initializer_list<LintTestCase> kTestCases = {
      // No Module Name at all
      {{kToken, "module"}, " foo; endmodule"},
      // Has comment but no Module Name
      {"// Some comment\n", {kToken, "module"}, " foo; endmodule"},
      {"// Copyright notice\n", {kToken, "module"}, " foo; endmodule"},
      {"// File description\n", {kToken, "module"}, " foo; endmodule"},
      // Case sensitive - lowercase should fail
      {"// module name: foo\n", {kToken, "module"}, " foo; endmodule"},
      {"// Module name: foo\n", {kToken, "module"}, " foo; endmodule"},
      {"// MODULE NAME: foo\n", {kToken, "module"}, " foo; endmodule"},
      // Partial match - should fail
      {"// Module\n", {kToken, "module"}, " foo; endmodule"},
      {"// Name:\n", {kToken, "module"}, " foo; endmodule"},
      {"// Module: foo\n", {kToken, "module"}, " foo; endmodule"},
  };
  RunLintTestCases<VerilogAnalyzer, ModuleDescriptionCheckRule>(kTestCases);
}

// Tests that files with Module Name AFTER module are rejected.
TEST(ModuleDescriptionCheckRuleTest, ModuleNameAfterModule) {
  constexpr int kToken = TK_OTHER;
  const std::initializer_list<LintTestCase> kTestCases = {
      // Module Name after module definition
      {{kToken, "module"}, " foo; endmodule\n// Module Name: foo"},
      // Module Name at end of file
      {{kToken, "module"},
       " foo; endmodule\n// Module Name: foo - Description"},
      // Module Name in middle of file
      {{kToken, "module"}, " foo; endmodule\n// Module Name: foo\n// Other"},
  };
  RunLintTestCases<VerilogAnalyzer, ModuleDescriptionCheckRule>(kTestCases);
}

// Tests that files without module definition should not trigger violation
TEST(ModuleDescriptionCheckRuleTest, NoModuleNoViolation) {
  const std::initializer_list<LintTestCase> kTestCases = {
      // Empty or whitespace files
      {""},
      {"\n"},
      {"   "},
      {"\n\n\n"},
      // Files with only comments
      {"// Some comment"},
      {"// Copyright notice"},
      {"// Just a comment\n// Another comment"},
      {"/* Block comment */"},
      {"/*\n * Multi-line\n * block comment\n */"},
      {"// Comment 1\n// Comment 2\n// Comment 3"},
  };
  RunLintTestCases<VerilogAnalyzer, ModuleDescriptionCheckRule>(kTestCases);
}

// Tests for modules with different syntax variations
TEST(ModuleDescriptionCheckRuleTest, ModuleVariations) {
  constexpr int kToken = TK_OTHER;
  const std::initializer_list<LintTestCase> kTestCases = {
      // Module with parameters
      {{kToken, "module"}, " #() foo; endmodule"},
      // Module with ports
      {{kToken, "module"}, " foo(input clk); endmodule"},
      // Module with parameters and ports
      {{kToken, "module"}, " #() bar(input a); endmodule"},
  };
  RunLintTestCases<VerilogAnalyzer, ModuleDescriptionCheckRule>(kTestCases);
}

// Tests for files with Module Name in correct position with module variations
TEST(ModuleDescriptionCheckRuleTest, AcceptsModuleVariations) {
  const std::initializer_list<LintTestCase> kTestCases = {
      // Module Name before parameterized module
      {"// Module Name: test_module\nmodule #() test_module; endmodule"},
      // Module Name before module with ports
      {"// Module Name: test_module\nmodule test_module(input clk); endmodule"},
      // Module Name before module with parameters and ports
      {"// Module Name: test_module\nmodule #() test_module(input a, output "
       "b); endmodule"},
  };
  RunLintTestCases<VerilogAnalyzer, ModuleDescriptionCheckRule>(kTestCases);
}

}  // namespace
}  // namespace analysis
}  // namespace verilog
