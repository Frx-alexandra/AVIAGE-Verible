module module_begin_block;
  wire w_foobar;
  begin   // LRM-invalid syntax
    wire w_barfoo;
  end
endmodule
