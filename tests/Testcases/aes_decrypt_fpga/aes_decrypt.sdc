# SDC file example
create_clock -period 2.5  clk 
set_output_delay 1 -clock clk [get_ports {*}]
set_pin_loc -pin inpad00 [get_ports clk]
