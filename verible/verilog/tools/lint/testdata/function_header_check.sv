// Test file for function-header-check rule

module function_header_check;
  // This is just a comment
  function automatic int my_function(input int a);
    return a + 1;
  endfunction
endmodule
