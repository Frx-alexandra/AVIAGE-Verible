// This file demonstrates wire/reg declaration alignment violations
// The rule requires that wire and reg declarations start from column 0

module wire_reg_declaration_alignment;
  // These declarations are indented and should fail
  wire [7:0] w_data_signal;
  wire [15:0] w_wide_data;
  reg [3:0] r_small_reg;
  wire w_single_bit;
endmodule
