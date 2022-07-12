//
// Copyright (c) 2018 QuickLogic Corporation.  All Rights Reserved.
//
// Description :
//    Example of a simple 16 bit up counter in Verilog HDL
//
// Version 1.0 : Initial Creation
//
module counter_16bit (clk, reset, enable, count);
input clk, reset, enable;
output [15:0] count;
reg [15:0] count;                                   

always @ (posedge clk)
if (reset == 1'b1) begin
  count <= 0;
end else if ( enable == 1'b1) begin
  count <= count + 1;
end

endmodule  
