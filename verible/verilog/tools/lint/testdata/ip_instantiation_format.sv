// This file demonstrates IP instantiation format violations

module ip_instantiation_format;
  logic clk1, rst1, data1;
  logic clk2, rst2, data_sig2;

  // First instance - this sets the expected format for the entire file
  ip_module_a inst1 (
    .clka(clk1),
    .rsta(rst1),
    .dataa(data1)
  );

  // Second instance - must match first instance format
  // Violation: different parenthesis positions (has space before opening paren)
  ip_module_b inst2 (
    .clkb (clk2),
    .rstb (rst2),
    .datab (data_sig2)
  );

endmodule
