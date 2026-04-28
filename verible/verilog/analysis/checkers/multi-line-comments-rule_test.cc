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

#include "absl/status/status.h"
#include "gtest/gtest.h"
#include "verible/common/analysis/linter-test-utils.h"
#include "verible/common/analysis/text-structure-linter-test-utils.h"
#include "verible/verilog/analysis/verilog-analyzer.h"
#include "verible/verilog/parser/verilog-token-enum.h"

namespace verilog {
namespace analysis {
namespace {

using verible::LintTestCase;
using verible::RunConfiguredLintTestCases;
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
// Default sign_count is 0, so any matching borders are accepted
TEST(MultiLineCommentsRuleTest, AcceptsProperMultiLine) {
  const std::
      initializer_list<LintTestCase>
          kTestCases =
              {
                  {"//"
                   "=========================================================="
                   "\n"  // 60 chars
                   "// comment line 1\n"
                   "//"
                   "=========================================================="
                   "\n"},  // 60 chars
                  {"//"
                   "=========================================================="
                   "\n"  // 60 chars
                   "// line 1\n"
                   "// line 2\n"
                   "//"
                   "=========================================================="
                   "\n"},  // 60 chars
                  {"//"
                   "=========================================================="
                   "\n"  // 60 chars
                   "// line 1\n"
                   "// line 2\n"
                   "// line 3\n"
                   "// line 4\n"
                   "//"
                   "=========================================================="
                   "\n"},  // 60 chars
                  // Different lengths are OK when sign_count=0 (default)
                  {"//====\n"  // Short border
                   "// comment\n"
                   "//====\n"},
                  {"//"
                   "==========================================================="
                   "=\n"  // 62 chars
                   "// comment\n"
                   "//"
                   "==========================================================="
                   "=\n"},
              };
  RunLintTestCases<VerilogAnalyzer, MultiLineCommentsRule>(kTestCases);
}

// Tests that improperly formatted multi-line comments are rejected.
// With default sign_count=0, borders must match but length doesn't matter
TEST(MultiLineCommentsRuleTest, RejectsImproperMultiLine) {
  const std::
      initializer_list<LintTestCase>
          kTestCases =
              {
                  // Missing proper closing border
                  {
                      "//"
                      "========================================================"
                      "==\n"  // 60 chars
                      "// comment line 1\n"
                      "// not a border\n",
                      {TK_OTHER, "// not a border"},
                  },
                  // Missing proper opening border
                  {
                      "// not a border\n"
                      "// comment line 1\n"
                      "//"
                      "========================================================"
                      "==\n",  // 60 chars
                      {TK_OTHER, "// comment line 1"},
                  },
                  // Different length borders (opening != closing)
                  {
                      "//========\n"  // Short
                      "// comment line 1\n"
                      "//================\n",  // Longer
                      {TK_OTHER, "//================"},
                  },
                  // Different characters (opening '=' vs closing '-')
                  {
                      "//====\n"
                      "// comment\n"
                      "//----\n",
                      {TK_OTHER, "//----"},
                  },
              };
  RunLintTestCases<VerilogAnalyzer, MultiLineCommentsRule>(kTestCases);
}

// Tests that all allowed characters (=, -, *, #, /) work properly
TEST(MultiLineCommentsRuleTest, AcceptsDifferentAllowedCharacters) {
  const std::
      initializer_list<LintTestCase>
          kTestCases =
              {
                  // Using '=' character
                  {"//"
                   "=========================================================="
                   "\n"  // 60 chars
                   "// comment with equals\n"
                   "//"
                   "=========================================================="
                   "\n"},
                  // Using '-' character
                  {"//"
                   "----------------------------------------------------------"
                   "\n"  // 60 chars
                   "// comment with dashes\n"
                   "//"
                   "----------------------------------------------------------"
                   "\n"},
                  // Using '*' character
                  {"//"
                   "**********************************************************"
                   "\n"  // 60 chars
                   "// comment with asterisks\n"
                   "//"
                   "**********************************************************"
                   "\n"},
                  // Using '#' character
                  {"//"
                   "##########################################################"
                   "\n"  // 60 chars
                   "// comment with hashes\n"
                   "//"
                   "##########################################################"
                   "\n"},
                  // Using '/' character
                  {"///////////////////////////////////////////////////////////"
                   "/\n"  // 60 chars
                   "// comment with slashes\n"
                   "///////////////////////////////////////////////////////////"
                   "/\n"},
                  // Shorter borders are OK too (default sign_count=0)
                  {"//===\n"
                   "// short\n"
                   "//===\n"},
              };
  RunLintTestCases<VerilogAnalyzer, MultiLineCommentsRule>(kTestCases);
}

// Tests that disallowed characters are rejected
TEST(MultiLineCommentsRuleTest, RejectsDisallowedCharacters) {
  const std::
      initializer_list<LintTestCase>
          kTestCases =
              {
                  // Using '~' character (not allowed)
                  {
                      "//"
                      "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
                      "~~\n"  // 60 chars
                      "// comment with tildes\n"
                      "//"
                      "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
                      "~~\n",
                      {TK_OTHER,
                       "//"
                       "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
                       "~~~"},
                  },
                  // Using '_' character (not allowed)
                  {
                      "//"
                      "________________________________________________________"
                      "__\n"  // 60 chars
                      "// comment with underscores\n"
                      "//"
                      "________________________________________________________"
                      "__\n",
                      {TK_OTHER,
                       "//"
                       "_______________________________________________________"
                       "___"},
                  },
                  // Using '.' character (not allowed)
                  {
                      "//"
                      "........................................................"
                      "..\n"  // 60 chars
                      "// comment with dots\n"
                      "//"
                      "........................................................"
                      "..\n",
                      {TK_OTHER,
                       "//"
                       "......................................................."
                       "..."},
                  },
              };
  RunLintTestCases<VerilogAnalyzer, MultiLineCommentsRule>(kTestCases);
}

// Tests that opening and closing borders must use the same character
TEST(MultiLineCommentsRuleTest, RejectsDifferentCharacters) {
  const std::
      initializer_list<LintTestCase>
          kTestCases =
              {
                  // Opening with '=', closing with '-'
                  {
                      "//"
                      "========================================================"
                      "==\n"  // 60 chars
                      "// comment line\n"
                      "//"
                      "--------------------------------------------------------"
                      "--\n",  // 60 chars
                      {TK_OTHER,
                       "//"
                       "-------------------------------------------------------"
                       "---"},
                  },
                  // Opening with '*', closing with '#'
                  {
                      "//"
                      "********************************************************"
                      "**\n"  // 60 chars
                      "// comment line\n"
                      "//"
                      "########################################################"
                      "##\n",  // 60 chars
                      {TK_OTHER,
                       "//"
                       "#######################################################"
                       "###"},
                  },
                  // Opening with '-', closing with '='
                  {
                      "//"
                      "--------------------------------------------------------"
                      "--\n"  // 60 chars
                      "// comment line\n"
                      "//"
                      "========================================================"
                      "==\n",  // 60 chars
                      {TK_OTHER,
                       "//"
                       "======================================================="
                       "==="},
                  },
              };
  RunLintTestCases<VerilogAnalyzer, MultiLineCommentsRule>(kTestCases);
}

// Tests that mixed characters in a single border are rejected
TEST(MultiLineCommentsRuleTest, RejectsMixedCharacters) {
  const std::initializer_list<LintTestCase> kTestCases = {
      // Mixed '=' and '-' in border
      {
          "//==========---------------=============================\n"  // 60
                                                                        // chars
                                                                        // (but
                                                                        // mixed)
          "// comment line\n"
          "//==========---------------=============================\n",
          {TK_OTHER,
           "//==========---------------============================="},
      },
      // Mixed '*' and '#' in border
      {
          "//**************************############################\n"  // 60
                                                                        // chars
                                                                        // (but
                                                                        // mixed)
          "// comment line\n"
          "//**************************############################\n",
          {TK_OTHER,
           "//**************************############################"},
      },
  };
  RunLintTestCases<VerilogAnalyzer, MultiLineCommentsRule>(kTestCases);
}

// Tests custom sign_count configuration
TEST(MultiLineCommentsRuleTest, CustomSignCountConfiguration) {
  // Test with sign_count = 40 (// + 38 characters)
  const std::initializer_list<LintTestCase> kTestCases40 = {
      // Valid with 40 total chars
      {"//======================================\n"  // 40 chars
       "// comment\n"
       "//======================================\n"},
      // Valid with 40 total chars using '-'
      {"//--------------------------------------\n"  // 40 chars
       "// comment\n"
       "//--------------------------------------\n"},
  };
  RunConfiguredLintTestCases<VerilogAnalyzer, MultiLineCommentsRule>(
      kTestCases40, "sign_count:40");

  // Test with sign_count = 80 (// + 78 characters)
  const std::initializer_list<LintTestCase> kTestCases80 = {
      // Valid with 80 total chars
      {"//"
       "======================================================================="
       "=======\n"  // 80 chars
       "// comment\n"
       "//"
       "======================================================================="
       "=======\n"},
  };
  RunConfiguredLintTestCases<VerilogAnalyzer, MultiLineCommentsRule>(
      kTestCases80, "sign_count:80");

  // Test with sign_count = 60 (explicit, // + 58 characters)
  const std::
      initializer_list<LintTestCase>
          kTestCases60 =
              {
                  // Valid with 60 total chars
                  {"//"
                   "=========================================================="
                   "\n"  // 60 chars
                   "// comment\n"
                   "//"
                   "=========================================================="
                   "\n"},
              };
  RunConfiguredLintTestCases<VerilogAnalyzer, MultiLineCommentsRule>(
      kTestCases60, "sign_count:60");
}

// Tests that wrong length fails with custom sign_count
TEST(MultiLineCommentsRuleTest, CustomSignCountRejectsWrongLength) {
  // Configured for sign_count = 40, but providing 60-char borders
  const std::
      initializer_list<LintTestCase>
          kTestCases =
              {
                  {
                      "//"
                      "========================================================"
                      "==\n"  // 60 chars (wrong!)
                      "// comment\n"
                      "//"
                      "========================================================"
                      "==\n",
                      {TK_OTHER,
                       "//"
                       "======================================================="
                       "==="},
                  },
              };
  RunConfiguredLintTestCases<VerilogAnalyzer, MultiLineCommentsRule>(
      kTestCases, "sign_count:40");
}

// Tests that configuration works independently
TEST(MultiLineCommentsRuleTest, ConfigurationIndependence) {
  absl::Status status;
  MultiLineCommentsRule rule;

  // Test valid configurations
  EXPECT_TRUE((status = rule.Configure("")).ok()) << status.message();
  EXPECT_TRUE((status = rule.Configure("sign_count:0")).ok())
      << status.message();  // No length check
  EXPECT_TRUE((status = rule.Configure("sign_count:40")).ok())
      << status.message();
  EXPECT_TRUE((status = rule.Configure("sign_count:60")).ok())
      << status.message();
  EXPECT_TRUE((status = rule.Configure("sign_count:100")).ok())
      << status.message();
}

// Tests the default behavior (sign_count = 0, no length enforcement)
TEST(MultiLineCommentsRuleTest, DefaultNoLengthCheck) {
  const std::initializer_list<LintTestCase> kTestCases = {
      // Various lengths, all should pass as long as borders match
      {"//==\n"
       "// short\n"
       "//==\n"},
      {"//=====\n"
       "// medium\n"
       "//=====\n"},
      {"//==========================================================\n"
       "// long\n"
       "//==========================================================\n"},
      {"//"
       "======================================================================="
       "=========\n"
       "// very long\n"
       "//"
       "======================================================================="
       "=========\n"},
  };
  RunLintTestCases<VerilogAnalyzer, MultiLineCommentsRule>(kTestCases);
}

// Tests explicit sign_count=0 configuration (same as default)
TEST(MultiLineCommentsRuleTest, ExplicitNoLengthCheck) {
  const std::initializer_list<LintTestCase> kTestCases = {
      {"//===\n"
       "// any length OK\n"
       "//===\n"},
      {"//====================\n"
       "// any length OK\n"
       "//====================\n"},
  };
  RunConfiguredLintTestCases<VerilogAnalyzer, MultiLineCommentsRule>(
      kTestCases, "sign_count:0");
}

// Tests that CRLF line endings (Windows) work correctly
TEST(MultiLineCommentsRuleTest, AcceptsCRLFLineEndings) {
  const std::initializer_list<LintTestCase> kTestCases = {
      // Valid multi-line comment with CRLF endings
      {"//==========================================================\r\n"
       "// comment line 1\r\n"
       "//==========================================================\r\n"},
      // Multiple content lines with CRLF
      {"//====\r\n"
       "// line 1\r\n"
       "// line 2\r\n"
       "//====\r\n"},
      // Mixed content with CRLF
      {"//------------------------------\r\n"
       "// Description here\r\n"
       "// More text\r\n"
       "//------------------------------\r\n"},
  };
  RunLintTestCases<VerilogAnalyzer, MultiLineCommentsRule>(kTestCases);
}

// Tests that mixed line endings work correctly
TEST(MultiLineCommentsRuleTest, AcceptsMixedLineEndings) {
  const std::initializer_list<LintTestCase> kTestCases = {
      // CRLF opening, LF content, CRLF closing
      {"//====\r\n"
       "// comment\n"
       "//====\r\n"},
      // Old Mac CR endings
      {"//====\r"
       "// comment\r"
       "//====\r"},
  };
  RunLintTestCases<VerilogAnalyzer, MultiLineCommentsRule>(kTestCases);
}

// Tests that CRLF line endings work with sign_count
TEST(MultiLineCommentsRuleTest, CRLFWithSignCount) {
  const std::initializer_list<LintTestCase> kTestCases = {
      // 40 chars with CRLF
      {"//======================================\r\n"
       "// comment\r\n"
       "//======================================\r\n"},
  };
  RunConfiguredLintTestCases<VerilogAnalyzer, MultiLineCommentsRule>(
      kTestCases, "sign_count:40");
}

}  // namespace
}  // namespace analysis
}  // namespace verilog