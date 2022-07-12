//
// Copyright (c) 2018 QuickLogic Corporation.  All Rights Reserved.
//
// Description :
//    testbench for simple 16 bit up counter in Verilog HDL
//
// Version 1.0 : Initial Creation
//

`timescale 10ns /10ps

module counter_16bit_tb; 
  reg clk, reset, enable; 
  wire [15:0] count; 
 
reg [15:0] count_compare; 

counter_16bit DUT (.clk(clk), .reset(reset), .enable(enable), .count(count)); 

event terminate_sim;  
  initial begin  
  @ (terminate_sim); 
    #5 $finish; 
  end 

always @ (posedge clk) 
if (reset == 1'b1) begin
  count_compare <= 0; 
end else if ( enable == 1'b1) begin
  count_compare <= count_compare + 1; 
end

  initial begin
    clk = 0; 
    reset = 1; 
   	enable = 0;
	#50 reset = 0;
	#50 enable = 1;
  end 
    
  always  
    #15 clk = !clk;     
  
  always @ (posedge clk) 
  if (count_compare != count) begin 
    $display ("DUT Error at time %d", $time); 
    $display (" Expected value %d, Got Value %d", count_compare, count); 
    #5 -> terminate_sim; 
  end 

  initial  begin
    $display("\t\ttime,\tclk,\treset,\tenable,\tcount"); 
    $monitor("%d,\t%b,\t%b,\t%b,\t%d",$time, clk,reset,enable,count); 
  end 

  initial 
  #5000 $finish;     
  
endmodule
