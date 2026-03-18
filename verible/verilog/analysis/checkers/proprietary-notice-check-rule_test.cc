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

#include "verible/verilog/analysis/checkers/proprietary-notice-check-rule.h"

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

// Tests that files with PROPRIETARY NOTICE before module are accepted.
TEST(ProprietaryNoticeCheckRuleTest, AcceptsText) {
  const std::initializer_list<LintTestCase> kTestCases = {
      // Single line comment with notice before module
      {"// PROPRIETARY NOTICE\nmodule foo; endmodule"},
      {"// This file contains PROPRIETARY NOTICE\nmodule foo; endmodule"},
      {"// PROPRIETARY NOTICE - Confidential\nmodule foo; endmodule"},
      // Block comment with notice before module
      {"/* PROPRIETARY NOTICE */\nmodule foo; endmodule"},
      {"/*\n * PROPRIETARY NOTICE\n * Confidential\n */\nmodule foo; "
       "endmodule"},
      // Multiple comments before module
      {"// Copyright 2024\n// PROPRIETARY NOTICE\nmodule foo; endmodule"},
      {"// Comment 1\n// Comment 2\n// PROPRIETARY NOTICE\nmodule foo; "
       "endmodule"},
      // Notice with special characters
      {"// PROPRIETARY NOTICE: Confidential & Proprietary\nmodule foo; "
       "endmodule"},
      {"// PROPRIETARY NOTICE - Company Name (c) 2024\nmodule foo; endmodule"},
  };
  RunLintTestCases<VerilogAnalyzer, ProprietaryNoticeCheckRule>(kTestCases);
}

// Tests that files without PROPRIETARY NOTICE are rejected.
TEST(ProprietaryNoticeCheckRuleTest, RejectsText) {
  constexpr int kToken = TK_OTHER;
  const std::initializer_list<LintTestCase> kTestCases = {
      // No notice at all
      {{kToken, "module"}, " foo; endmodule"},
      // Has comment but no PROPRIETARY NOTICE
      {"// Some comment\n", {kToken, "module"}, " foo; endmodule"},
      {"// Copyright notice\n", {kToken, "module"}, " foo; endmodule"},
      {"// File description\n", {kToken, "module"}, " foo; endmodule"},
      // Case sensitive - lowercase should fail
      {"// Proprietary Notice\n", {kToken, "module"}, " foo; endmodule"},
      {"// proprietary notice\n", {kToken, "module"}, " foo; endmodule"},
      {"// PROPRIETARY notice\n", {kToken, "module"}, " foo; endmodule"},
      {"// proprietary NOTICE\n", {kToken, "module"}, " foo; endmodule"},
      // Partial match - should fail
      {"// PROPRIETARY\n", {kToken, "module"}, " foo; endmodule"},
      {"// NOTICE\n", {kToken, "module"}, " foo; endmodule"},
  };
  RunLintTestCases<VerilogAnalyzer, ProprietaryNoticeCheckRule>(kTestCases);
}

// Tests that files with notice AFTER module are rejected.
TEST(ProprietaryNoticeCheckRuleTest, NoticeAfterModule) {
  constexpr int kToken = TK_OTHER;
  const std::initializer_list<LintTestCase> kTestCases = {
      // Notice after module definition
      {{kToken, "module"}, " foo; endmodule\n// PROPRIETARY NOTICE"},
      // Notice at end of file
      {{kToken, "module"}, " foo; endmodule\n// PROPRIETARY NOTICE - Late"},
      // Notice in middle of file
      {{kToken, "module"}, " foo; endmodule\n// PROPRIETARY NOTICE\n// Other"},
  };
  RunLintTestCases<VerilogAnalyzer, ProprietaryNoticeCheckRule>(kTestCases);
}

// Tests that files without module definition should not trigger violation
TEST(ProprietaryNoticeCheckRuleTest, NoModuleNoViolation) {
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
  RunLintTestCases<VerilogAnalyzer, ProprietaryNoticeCheckRule>(kTestCases);
}

// Tests for modules with different syntax variations
TEST(ProprietaryNoticeCheckRuleTest, ModuleVariations) {
  constexpr int kToken = TK_OTHER;
  const std::initializer_list<LintTestCase> kTestCases = {
      // Module with parameters
      {{kToken, "module"}, " #() foo; endmodule"},
      // Module with ports
      {{kToken, "module"}, " foo(input clk); endmodule"},
      // Module with parameters and ports
      {{kToken, "module"}, " #() bar(input a); endmodule"},
  };
  RunLintTestCases<VerilogAnalyzer, ProprietaryNoticeCheckRule>(kTestCases);
}

// Tests for files with notice in correct position with module variations
TEST(ProprietaryNoticeCheckRuleTest, AcceptsModuleVariations) {
  const std::initializer_list<LintTestCase> kTestCases = {
      // Notice before parameterized module
      {"// PROPRIETARY NOTICE\nmodule #() foo; endmodule"},
      // Notice before module with ports
      {"// PROPRIETARY NOTICE\nmodule bar(input clk); endmodule"},
      // Notice before module with parameters and ports
      {"// PROPRIETARY NOTICE\nmodule #() baz(input a, output b); endmodule"},
  };
  RunLintTestCases<VerilogAnalyzer, ProprietaryNoticeCheckRule>(kTestCases);
}

}  // namespace
}  // namespace analysis
}  // namespace verilog
