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
      // Has MODIFICATION HISTORY before module - should pass
      {"// MODIFICATION HISTORY\nmodule foo; endmodule"},
      {"// This file contains MODIFICATION HISTORY\nmodule foo; endmodule"},
      {"// MODIFICATION HISTORY - Version 1.0\n\nmodule foo; endmodule"},
      // Has MODIFICATION HISTORY in block comment before module - should pass
      {"/* MODIFICATION HISTORY */\nmodule foo; endmodule"},
      {"/*\n * MODIFICATION HISTORY\n * Version 1.0\n */\nmodule foo; "
       "endmodule"},
      // Multiple comments before module - should pass
      {"// Copyright 2024\n// MODIFICATION HISTORY\nmodule foo; endmodule"},
  };
  RunLintTestCases<VerilogAnalyzer, ModificationHistoryCheckRule>(kTestCases);
}

// Tests that files without MODIFICATION HISTORY or with it after module are
// rejected.
TEST(ModificationHistoryCheckRuleTest, RejectsText) {
  constexpr int kToken = TK_OTHER;
  const std::initializer_list<LintTestCase> kTestCases = {
      // No history at all
      {"module foo; endmodule", {kToken, ""}},
      // Has comment but no MODIFICATION HISTORY - violation at end of first
      // line
      {"// Some comment", {kToken, ""}},
      {"// Copyright notice", {kToken, ""}},
      // Case sensitive - lowercase should fail
      {"// Modification History", {kToken, ""}},
      {"// modification history", {kToken, ""}},
      // Partial match - should fail
      {"// MODIFICATION", {kToken, ""}},
      {"// HISTORY", {kToken, ""}},
  };
  RunLintTestCases<VerilogAnalyzer, ModificationHistoryCheckRule>(kTestCases);
}

}  // namespace
}  // namespace analysis
}  // namespace verilog
