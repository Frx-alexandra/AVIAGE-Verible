module mismatched_labels(
  input i_clk_i
);

always_ff @(posedge i_clk_i)
  begin : foo
  end : bar // This mismatched label should cause an error

endmodule;
