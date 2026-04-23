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

#include "verible/verilog/analysis/checkers/task-header-check-rule.h"

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

TEST(TaskHeaderCheckTest, ValidTaskHeader) {
  const std::initializer_list<LintTestCase> kTestCases = {
      // Valid: TASK in comment before task
      {"// TASK: my_task\n"
       "task my_task;\n"
       "endtask"},

      // Valid: TASK in block comment before task
      {"/* TASK: my_task */\n"
       "task my_task;\n"
       "endtask"},

      // Valid: TASK in comment with extra text
      {"// This is a TASK declaration\n"
       "task my_task;\n"
       "endtask"},

      // Valid: Multiple tasks with headers
      {"// TASK: task1\n"
       "task task1;\n"
       "endtask\n"
       "// TASK: task2\n"
       "task task2;\n"
       "endtask"},

      // Valid: No task - no violation
      {"module test;\n"
       "endmodule"},

      // Valid: Task in module with header
      {"module test;\n"
       "// TASK: calc\n"
       "task calc(input a);\n"
       "endtask\n"
       "endmodule"},
  };

  RunLintTestCases<VerilogAnalyzer, TaskHeaderCheckRule>(kTestCases);
}

TEST(TaskHeaderCheckTest, InvalidTaskHeader) {
  constexpr int kToken = TK_OTHER;
  const std::initializer_list<LintTestCase> kTestCases = {
      // Invalid: No TASK comment before task
      {"",
       {kToken, "task"},
       " my_task;\n"
       "endtask"},

      // Invalid: Wrong keyword in comment
      {"// This is a module\n"
       "",
       {kToken, "task"},
       " my_task;\n"
       "endtask"},

      // Invalid: TASK after task
      {"",
       {kToken, "task"},
       " my_task;\n"
       "endtask\n"
       "// TASK: my_task"},

      // Invalid: TASK in non-comment text
      {"string s = \"This is a TASK\";\n"
       "",
       {kToken, "task"},
       " my_task;\n"
       "endtask"},

      // Invalid: Multiple tasks, second one missing header
      {"// TASK: task1\n"
       "task task1;\n"
       "endtask\n"
       "",
       {kToken, "task"},
       " task2;\n"
       "endtask"},
  };

  RunLintTestCases<VerilogAnalyzer, TaskHeaderCheckRule>(kTestCases);
}

TEST(TaskHeaderCheckTest, EdgeCases) {
  constexpr int kToken = TK_OTHER;
  const std::initializer_list<LintTestCase> kTestCases = {
      // Valid: TASK with different case
      {"// task: my_task\n"
       "task my_task;\n"
       "endtask"},

      // Valid: TASK in inline comment
      {"task my_task; // TASK\n"
       "endtask"},

      // Invalid: Empty file
      {""},

      // Valid: Task with only whitespace before
      {"   \n"
       "// TASK: test\n"
       "task test;\n"
       "endtask"},

      // Valid: Task with blank lines before header
      {"\n"
       "\n"
       "// TASK: test\n"
       "task test;\n"
       "endtask"},

      // Invalid: Task immediately after module declaration
      {"module test;\n"
       "",
       {kToken, "task"},
       " calc();\n"
       "endtask\n"
       "endmodule"},
  };

  RunLintTestCases<VerilogAnalyzer, TaskHeaderCheckRule>(kTestCases);
}

}  // namespace
}  // namespace analysis
}  // namespace verilog