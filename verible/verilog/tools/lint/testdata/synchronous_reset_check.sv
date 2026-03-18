// Test file for synchronous-reset-check rule

module synchronous_reset_check;
  reg r_q, r_d;
  wire w_clk, w_rst;

  always @(posedge w_clk or posedge w_rst) begin
    if (i_rst) begin
      r_q <= 1'b0;
    end else  begin
      r_q <= r_d;
    end
  end
endmodule
