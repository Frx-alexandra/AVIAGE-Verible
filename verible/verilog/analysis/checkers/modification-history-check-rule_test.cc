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

#include "verible/verilog/analysis/checkers/modification-history-check-rule.h"

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

// Tests that files with MODIFICATION HISTORY before module are accepted.
TEST(ModificationHistoryCheckRuleTest, AcceptsText) {
  const std::initializer_list<LintTestCase> kTestCases = {
      // Single line comment with history before module
      {"// MODIFICATION HISTORY\nmodule foo; endmodule"},
      {"// This file contains MODIFICATION HISTORY\nmodule foo; endmodule"},
      {"// MODIFICATION HISTORY - Version 1.0\nmodule foo; endmodule"},
      // Block comment with history before module
      {"/* MODIFICATION HISTORY */\nmodule foo; endmodule"},
      {"/*\n * MODIFICATION HISTORY\n * Version 1.0\n */\nmodule foo; "
       "endmodule"},
      // Multiple comments before module
      {"// Copyright 2024\n// MODIFICATION HISTORY\nmodule foo; endmodule"},
      {"// Comment 1\n// Comment 2\n// MODIFICATION HISTORY\nmodule foo; "
       "endmodule"},
      // History with version info
      {"// MODIFICATION HISTORY: v1.0 - Initial release\nmodule foo; "
       "endmodule"},
      {"// MODIFICATION HISTORY - 2024-01-01: Created\nmodule foo; endmodule"},
  };
  RunLintTestCases<VerilogAnalyzer, ModificationHistoryCheckRule>(kTestCases);
}

// Tests that files without MODIFICATION HISTORY are rejected.
TEST(ModificationHistoryCheckRuleTest, RejectsText) {
  constexpr int kToken = TK_OTHER;
  const std::initializer_list<LintTestCase> kTestCases = {
      // No history at all
      {{kToken, "module"}, " foo; endmodule"},
      // Has comment but no MODIFICATION HISTORY
      {"// Some comment\n", {kToken, "module"}, " foo; endmodule"},
      {"// Copyright notice\n", {kToken, "module"}, " foo; endmodule"},
      {"// File description\n", {kToken, "module"}, " foo; endmodule"},
      // Case sensitive - lowercase should fail
      {"// Modification History\n", {kToken, "module"}, " foo; endmodule"},
      {"// modification history\n", {kToken, "module"}, " foo; endmodule"},
      {"// MODIFICATION history\n", {kToken, "module"}, " foo; endmodule"},
      {"// modification HISTORY\n", {kToken, "module"}, " foo; endmodule"},
      // Partial match - should fail
      {"// MODIFICATION\n", {kToken, "module"}, " foo; endmodule"},
      {"// HISTORY\n", {kToken, "module"}, " foo; endmodule"},
  };
  RunLintTestCases<VerilogAnalyzer, ModificationHistoryCheckRule>(kTestCases);
}

// Tests that files with history AFTER module are rejected.
TEST(ModificationHistoryCheckRuleTest, HistoryAfterModule) {
  constexpr int kToken = TK_OTHER;
  const std::initializer_list<LintTestCase> kTestCases = {
      // History after module definition
      {{kToken, "module"}, " foo; endmodule\n// MODIFICATION HISTORY"},
      // History at end of file
      {{kToken, "module"}, " foo; endmodule\n// MODIFICATION HISTORY - v2.0"},
      // History in middle of file
      {{kToken, "module"},
       " foo; endmodule\n// MODIFICATION HISTORY\n// Other"},
  };
  RunLintTestCases<VerilogAnalyzer, ModificationHistoryCheckRule>(kTestCases);
}

// Tests that files without module definition should not trigger violation
TEST(ModificationHistoryCheckRuleTest, NoModuleNoViolation) {
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
  RunLintTestCases<VerilogAnalyzer, ModificationHistoryCheckRule>(kTestCases);
}

// Tests for modules with different syntax variations
TEST(ModificationHistoryCheckRuleTest, ModuleVariations) {
  constexpr int kToken = TK_OTHER;
  const std::initializer_list<LintTestCase> kTestCases = {
      // Module with parameters
      {{kToken, "module"}, " #() foo; endmodule"},
      // Module with ports
      {{kToken, "module"}, " foo(input clk); endmodule"},
      // Module with parameters and ports
      {{kToken, "module"}, " #() bar(input a); endmodule"},
  };
  RunLintTestCases<VerilogAnalyzer, ModificationHistoryCheckRule>(kTestCases);
}

// Tests for files with history in correct position with module variations
TEST(ModificationHistoryCheckRuleTest, AcceptsModuleVariations) {
  const std::initializer_list<LintTestCase> kTestCases = {
      // History before parameterized module
      {"// MODIFICATION HISTORY\nmodule #() foo; endmodule"},
      // History before module with ports
      {"// MODIFICATION HISTORY\nmodule bar(input clk); endmodule"},
      // History before module with parameters and ports
      {"// MODIFICATION HISTORY\nmodule #() baz(input a, output b); endmodule"},
  };
  RunLintTestCases<VerilogAnalyzer, ModificationHistoryCheckRule>(kTestCases);
}

}  // namespace
}  // namespace analysis
}  // namespace verilog
