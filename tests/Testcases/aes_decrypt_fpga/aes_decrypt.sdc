# SDC file example
create_clock -period 10 -name clk_v0 [get_ports clk]
set_pin_loc -pin inpad00 [get_ports clk]
