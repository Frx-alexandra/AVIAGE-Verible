// This file demonstrates wire/reg declaration alignment violations

module wire_reg_declaration_alignment;
  // First declaration sets the style
  wire [7:0] w_data_signal;

  // These declarations have inconsistent alignment and should fail
  wire [15:0] w_wide_data;
  reg [3:0] r_small_reg;
  wire w_single_bit;

endmodule
