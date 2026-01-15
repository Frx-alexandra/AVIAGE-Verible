module begin_same_line();
  wire condition;

  // Valid cases - begin on same line
  if (condition) begin : gen_valid
    // valid
  end

  // Invalid cases - begin on different line (should trigger violations)
  if (condition)
  begin : gen_invalid
    // invalid
  end
endmodule
