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

#include "verible/verilog/analysis/checkers/multi-line-comments-rule.h"

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

// Tests that single line comments pass.
TEST(MultiLineCommentsRuleTest, AcceptsSingleLine) {
  const std::initializer_list<LintTestCase> kTestCases = {
      {"// single line comment\n"},
      {"// another comment\n"},
      {"code; // inline comment\n"},
  };
  RunLintTestCases<VerilogAnalyzer, MultiLineCommentsRule>(kTestCases);
}

// Tests that properly formatted multi-line comments pass.
TEST(MultiLineCommentsRuleTest, AcceptsProperMultiLine) {
  const std::initializer_list<LintTestCase> kTestCases = {
      {"//===========================================================\n"
       "// comment line 1\n"
       "//===========================================================\n"},
      {"//===========================================================\n"
       "// line 1\n"
       "// line 2\n"
       "//===========================================================\n"},
  };
  RunLintTestCases<VerilogAnalyzer, MultiLineCommentsRule>(kTestCases);
}

// Tests that improperly formatted multi-line comments are rejected.
TEST(MultiLineCommentsRuleTest, RejectsImproperMultiLine) {
  const std::initializer_list<LintTestCase> kTestCases = {
      {
          "//===========================================================\n"
          "// comment line 1\n"
          "// not a border\n",
          {TK_OTHER, "// not a border"},
      },
      {
          "// not a border\n"
          "// comment line 1\n"
          "//===========================================================\n",
          {TK_OTHER, "// comment line 1"},
      },
      {
          "//===========================================================\n"
          "code; // not a comment\n"
          "//===========================================================\n",
          {TK_OTHER, "code; // not a comment"},
      },
  };
  RunLintTestCases<VerilogAnalyzer, MultiLineCommentsRule>(kTestCases);
}

}  // namespace
}  // namespace analysis
}  // namespace verilog