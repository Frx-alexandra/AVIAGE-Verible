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
      // Has PROPRIETARY NOTICE before module - should pass
      {"// PROPRIETARY NOTICE\nmodule foo; endmodule"},
      {"// This file contains PROPRIETARY NOTICE\nmodule foo; endmodule"},
      {"// PROPRIETARY NOTICE - Confidential\n\nmodule foo; endmodule"},
      // Has PROPRIETARY NOTICE in block comment before module - should pass
      {"/* PROPRIETARY NOTICE */\nmodule foo; endmodule"},
      {"/*\n * PROPRIETARY NOTICE\n * Confidential\n */\nmodule foo; "
       "endmodule"},
      // Multiple comments before module - should pass
      {"// Copyright 2024\n// PROPRIETARY NOTICE\nmodule foo; endmodule"},
  };
  RunLintTestCases<VerilogAnalyzer, ProprietaryNoticeCheckRule>(kTestCases);
}

// Tests that files without PROPRIETARY NOTICE or with it after module are
// rejected.
TEST(ProprietaryNoticeCheckRuleTest, RejectsText) {
  constexpr int kToken = TK_OTHER;
  const std::initializer_list<LintTestCase> kTestCases = {
      // No notice at all
      {"module foo; endmodule", {kToken, ""}},
      // Has comment but no PROPRIETARY NOTICE - violation at end of first line
      {"// Some comment", {kToken, ""}},
      {"// Copyright notice", {kToken, ""}},
      // Case sensitive - lowercase should fail
      {"// Proprietary Notice", {kToken, ""}},
      {"// proprietary notice", {kToken, ""}},
      // Partial match - should fail
      {"// PROPRIETARY", {kToken, ""}},
      {"// NOTICE", {kToken, ""}},
  };
  RunLintTestCases<VerilogAnalyzer, ProprietaryNoticeCheckRule>(kTestCases);
}

}  // namespace
}  // namespace analysis
}  // namespace verilog
