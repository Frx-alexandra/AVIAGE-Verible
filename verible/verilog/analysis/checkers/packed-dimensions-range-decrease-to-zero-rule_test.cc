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

#include "verible/verilog/analysis/checkers/packed-dimensions-range-decrease-to-zero-rule.h"

#include <initializer_list>

#include "gtest/gtest.h"
#include "verible/common/analysis/linter-test-utils.h"
#include "verible/common/analysis/syntax-tree-linter-test-utils.h"
#include "verible/verilog/analysis/verilog-analyzer.h"
#include "verible/verilog/parser/verilog-token-enum.h"

namespace verilog {
namespace analysis {
namespace {

using verible::LintTestCase;
using verible::RunLintTestCases;

TEST(PackedDimensionsRangeDecreaseToZeroRuleTests, CheckRanges) {
  constexpr int kType = TK_OTHER;
  // TODO(fangism): use NodeEnum::kDimensionRange

  const std::initializer_list<LintTestCase> kTestCases = {
      // Test basic wire declarations - flag descending ranges with RHS != 0
      {""},
      {"wire [0:0] w;"},                   // single bit, no violation
      {"wire [1:0] w;"},                   // correct [1:0], RHS=0, no violation
      {"wire [0:1] w;"},                   // ascending, no violation
      {"wire [", {kType, "2:1"}, "] w;"},  // descending, RHS=1 !=0, violation
      {"wire [1:2] w;"},                   // ascending, no violation
      {"wire [z:0] w;"},                   // non-constant, no violation
      {"wire [0:z] w;"},       // ascending, non-constant RHS, no violation
      {"wire [z:1] w;"},       // non-constant, no violation
      {"wire [1:0][2:0] w;"},  // correct [1:0] and [2:0], no violation
      {"wire [0:1][2:0] w;"},  // ascending [0:1], correct [2:0], no violation
                               // for either
      {"wire [1:0][",
       {kType, "2:1"},
       "] w;"},  // correct [1:0], violation for [2:1]

      // reg declarations
      {"reg [0:0] r;"},                   // single bit, no violation
      {"reg [1:0] r;"},                   // correct [1:0], RHS=0, no violation
      {"reg [0:1] r;"},                   // ascending, no violation
      {"reg [", {kType, "2:1"}, "] r;"},  // descending, RHS=1 !=0, violation
      {"reg [1:2] r;"},                   // ascending, no violation

      // module-local nets
      {"module m; endmodule"},
      {"module m; wire [0:0] w; endmodule"},  // single bit, no violation
      {"module m; wire [0:1] w; endmodule"},  // ascending, no violation
      {"module m; wire [1:0] w; endmodule"},  // correct, no violation
      {"module m; wire [1:1] w; endmodule"},  // single bit, no violation
      {"module m; wire [", {kType, "2:1"}, "] w [4]; endmodule"},  // violation

      // module-ports
      {"module m(input wire [0:0] w); endmodule"},  // single bit, no violation
      {"module m(input wire [0:1] w); endmodule"},  // ascending, no violation
      {"module m(input wire [1:0] w); endmodule"},  // correct, no violation
      {"module m(input wire [1:1] w); endmodule"},  // single bit, no violation
      {"module m(input wire [2:1] w [4]); endmodule"},  // not checking module
                                                        // ports

      // class members - not wire/reg, no violations
      {"class c; endclass"},
      {"class c; logic [0:0] l; endclass"},
      {"class c; logic [0:1] l; endclass"},
      {"class c; logic [1:0] l; endclass"},
      {"class c; logic [1:1] l; endclass"},

      // struct members - not wire/reg, no violations
      {"struct { bit [0:0] l; } s;"},
      {"struct { bit [0:1] l; } s;"},
      {"struct { bit [1:0] l; } s;"},
      {"struct { bit [x:y] l; } s;"},

      // struct typedef members - not wire/reg, no violations
      {"typedef struct { bit [0:0] l; } s_s;"},
      {"typedef struct { bit [0:1] l; } s_s;"},
      {"typedef struct { bit [1:0] l; } s_s;"},
      {"typedef struct { bit [x:y] l; } s_s;"},
  };
  RunLintTestCases<VerilogAnalyzer, PackedDimensionsRangeDecreaseToZeroRule>(
      kTestCases);
}

}  // namespace
}  // namespace analysis
}  // namespace verilog