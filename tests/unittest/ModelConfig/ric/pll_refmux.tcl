#!/usr/bin/env tclsh
####################################
# created by Alex H , April 4, 2023
# updated by George C, Oct 2023
# PLLREF_MUX definition 
####################################
define_block -name PLLREF_MUX

####################################
# define configuration attributes (parameters) 
####################################
# from IO_BANK_VirgoTC.docx
define_attr -block PLLREF_MUX -name cfg_pllref_hv_rx_io_sel      -addr 0 -width 1 -enumname PLLREF_MUX_ATT_0 -enum {pllref_hv_rx_io_sel_0 0} {pllref_hv_rx_io_sel_1 1}
define_attr -block PLLREF_MUX -name cfg_pllref_hv_bank_rx_io_sel -addr 1 -width 2
define_attr -block PLLREF_MUX -name cfg_pllref_hp_rx_io_sel      -addr 3 -width 2 -enumname PLLREF_MUX_ATT_2 -enum {pllref_hp_rx_io_sel_0 0} {pllref_hp_rx_io_sel_1 1} {pllref_hp_rx_io_sel_2 2} {pllref_hp_rx_io_sel_3 3}
define_attr -block PLLREF_MUX -name cfg_pllref_hp_bank_rx_io_sel -addr 5 -width 1 -enumname PLLREF_MUX_ATT_3 -enum {pllref_hp_bank_rx_io_sel_0 0} {pllref_hp_bank_rx_io_sel_1 1}
define_attr -block PLLREF_MUX -name cfg_pllref_use_hv            -addr 6 -width 1 -enumname PLLREF_MUX_ATT_4 -enum {pllref_use_hv_0 0} {pllref_use_hv_1 1}
define_attr -block PLLREF_MUX -name cfg_pllref_use_rosc          -addr 7 -width 1 -enumname PLLREF_MUX_ATT_5 -enum {pllref_use_rosc_0 0} {pllref_use_rosc_1 1}
define_attr -block PLLREF_MUX -name cfg_pllref_use_div           -addr 8 -width 1 -enumname PLLREF_MUX_ATT_6 -enum {pllref_use_div_0 0} {pllref_use_div_1 1}
##
####################################
# Constraints within block attributes 
####################################
define_constraint -block PLLREF_MUX -constraint {(cfg_pllref_hv_bank_rx_io_sel > 2) -> FALSE}

######################################
# Ports
# Not in diagram: define_ports -block PLLREF_MUX -in system_reset_n 
# Not in diagram: define_ports -block PLLREF_MUX -in cfg_done 
# Not in diagram: define_ports -block PLLREF_MUX -in rosc_clk 
define_ports -block PLLREF_MUX -in hp_rx_io_clk_0_1
define_ports -block PLLREF_MUX -in hp_rx_io_clk_0_0 
define_ports -block PLLREF_MUX -in hp_rx_io_clk_1_1
define_ports -block PLLREF_MUX -in hp_rx_io_clk_1_0 
define_ports -block PLLREF_MUX -in hv_rx_io_clk_0_1
define_ports -block PLLREF_MUX -in hv_rx_io_clk_0_0 
define_ports -block PLLREF_MUX -in hv_rx_io_clk_1_1
define_ports -block PLLREF_MUX -in hv_rx_io_clk_1_0 
define_ports -block PLLREF_MUX -in xin_clk_l_r
define_ports -block PLLREF_MUX -out pll_refmux_out 
######################################

######################################
# Define the connectivity table
# Setting the memory (in bits as layed out above)
# will connect the signal (input) before "._to_." 
# to the signal after it.
# Note that the table definition does not have to be in the same file
# it can be in another file and loaded.
# The addressing can also be loaded as of a predefined policy
######################################

define_properties -block PLLREF_MUX -____0____._to_.pll_refmux_out  "xxxxxx100"
define_properties -block PLLREF_MUX -hp_rx_io_clk_0_1._to_.pll_refmux_out  "xxx0x0000"
define_properties -block PLLREF_MUX -hp_rx_io_clk_0_0._to_.pll_refmux_out  "xxx1x0000" 
define_properties -block PLLREF_MUX -hp_rx_io_clk_1_1._to_.pll_refmux_out  "xxxx01000" 
define_properties -block PLLREF_MUX -hp_rx_io_clk_1_0._to_.pll_refmux_out  "xxxx11000"  
define_properties -block PLLREF_MUX -hv_rx_io_clk_0_1._to_.pll_refmux_out  "010xxx100" 
define_properties -block PLLREF_MUX -hv_rx_io_clk_0_0._to_.pll_refmux_out  "011xxx100"  
define_properties -block PLLREF_MUX -hv_rx_io_clk_1_1._to_.pll_refmux_out  "110xxx100" 
define_properties -block PLLREF_MUX -hv_rx_io_clk_1_0._to_.pll_refmux_out  "111xxx100"  
define_properties -block PLLREF_MUX -xin_clk_l_r._to_.pll_refmux_out       "xxxxxxx10"
define_properties -block PLLREF_MUX -____0_____DIVIDED._to_.pll_refmux_out "xxxxxx101" 
define_properties -block PLLREF_MUX -hp_rx_io_clk_0_1_DIVIDED._to_.pll_refmux_out "xxx0x0001" 
define_properties -block PLLREF_MUX -hp_rx_io_clk_0_0_DIVIDED._to_.pll_refmux_out "xxx1x0001"  
define_properties -block PLLREF_MUX -hp_rx_io_clk_1_1_DIVIDED._to_.pll_refmux_out "xxxx01001" 
define_properties -block PLLREF_MUX -hp_rx_io_clk_1_0_DIVIDED._to_.pll_refmux_out "xxxx11001"  
define_properties -block PLLREF_MUX -hv_rx_io_clk_0_1_DIVIDED._to_.pll_refmux_out "010xxx101" 
define_properties -block PLLREF_MUX -hv_rx_io_clk_0_0_DIVIDED._to_.pll_refmux_out "011xxx101"  
define_properties -block PLLREF_MUX -hv_rx_io_clk_1_1_DIVIDED._to_.pll_refmux_out "110xxx101" 
define_properties -block PLLREF_MUX -hv_rx_io_clk_1_0_DIVIDED._to_.pll_refmux_out "111xxx101"  
define_properties -block PLLREF_MUX -xin_clk_l_r_DIVIDED._to_.pll_refmux_out "xxxxxxx11"