module begin_same_line();
  wire w_condition;

  // Valid cases - begin on same line
  if (w_condition) begin : gen_valid
    // valid
  end

  // Invalid cases - begin on different line (should trigger violations)
  if (w_condition)
  begin : gen_invalid
    // invalid
  end
endmodule
