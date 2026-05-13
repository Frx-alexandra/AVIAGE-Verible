// Test file for conditional-operator-spacing rule
//
// KNOWN ISSUE: This rule currently does not work in the production linter.
// The rule implementation uses TokenStreamLintRule which receives filtered
// tokens (whitespace removed), making spacing calculation impossible.
// The rule works correctly in unit tests but not in verible-verilog-lint.
//
// This causes the onerule_test to fail with "Expected diagnostics, but found none."
// To fix: The rule needs to be converted to TextStructureLintRule or needs
// access to unfiltered token streams with position information.
//
// The violations below demonstrate what SHOULD be caught once the rule is fixed.

module conditional_operator_spacing;
  logic [7:0] a, b, c, d;
  logic result;

  // Violations: Unequal spacing around operators
  assign result = (a ==b);   // space before but not after
  assign result = (a== b);   // space after but not before
  assign result = (a !=b);
  assign result = (c!= d);
  assign result = (a <b);
  assign result = (c< d);
  assign result = (a >b);
  assign result = (c> d);
  assign result = (a <=b);
  assign result = (c<= d);
  assign result = (a >=b);
  assign result = (c>= d);
  assign result = (a ===b);
  assign result = (c=== d);
  assign result = (a !==b);
  assign result = (c!== d);

endmodule



