create_clock -period 1 -name clk
set_input_delay 1 -clock clk [get_ports [all_inputs] [all_outputs]]
set_output_delay 1 -clock clk [get_ports {*}]
