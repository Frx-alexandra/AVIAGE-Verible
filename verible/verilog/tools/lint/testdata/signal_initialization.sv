// Test file for signal-initialization rule

module signal_initialization;
  reg [7:0] r_counter;
  reg r_state, r_valid, r_error;
  wire w_clk, w_rst;

  // FAIL: r_counter not initialized in reset branch
  always @(posedge w_clk) begin
    if (w_rst) begin
      r_state <= 1'b0;
      // Missing: r_counter <= 8'h00;
    end else begin
      r_counter <= r_counter + 1;
      r_state <= r_counter[7];
    end
  end

  // FAIL: r_valid and r_error not initialized
  always @(posedge w_clk) begin
    if (w_rst) begin
      r_counter <= 8'h00;
      // Missing: r_valid <= 1'b0;
      // Missing: r_error <= 1'b0;
    end else begin
      r_counter <= r_counter + 1;
      r_valid <= 1'b1;
      r_error <= r_counter[7];
    end
  end
endmodule
