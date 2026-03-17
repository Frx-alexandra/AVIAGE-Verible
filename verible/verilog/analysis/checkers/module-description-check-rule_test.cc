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
      // Has Module Name before module - should pass
      {"// Module Name: test_module\nmodule test_module; endmodule"},
      {"// Module Name: my_design\nmodule my_design; endmodule"},
      {"// Module Name: test_module\n// Description: test\nmodule test_module; "
       "endmodule"},
      // Has Module Name in block comment before module - should pass
      {"/* Module Name: test_module */\nmodule test_module; endmodule"},
      {"/*\n * Module Name: test_module\n * Description: test\n */\nmodule "
       "test_module; endmodule"},
  };
  RunLintTestCases<VerilogAnalyzer, ModuleDescriptionCheckRule>(kTestCases);
}

// Tests that files without Module Name or with it after module are rejected.
TEST(ModuleDescriptionCheckRuleTest, RejectsText) {
  constexpr int kToken = TK_OTHER;
  const std::initializer_list<LintTestCase> kTestCases = {
      // No Module Name at all
      {"module foo; endmodule", {kToken, ""}},
      // Has comment but no Module Name
      {"// Some comment", {kToken, ""}},
      {"// Copyright notice", {kToken, ""}},
      // Case sensitive - lowercase should fail
      {"// module name: foo", {kToken, ""}},
      {"// Module name: foo", {kToken, ""}},
      // Partial match - should fail
      {"// Module", {kToken, ""}},
      {"// Name:", {kToken, ""}},
  };
  RunLintTestCases<VerilogAnalyzer, ModuleDescriptionCheckRule>(kTestCases);
}

}  // namespace
}  // namespace analysis
}  // namespace verilog
