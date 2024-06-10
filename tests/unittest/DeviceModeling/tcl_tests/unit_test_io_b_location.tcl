package require tcltest  ; # Make sure tcltest is loaded first
namespace import -force ::tcltest::*

# Test setup: Define blocks and create instances
define_block -name GBOX_HV_40X2_VL
define_block -name ROOT_BANK_CLKMUX
define_block -name FCLK_MUX
define_block -name b_l1
define_block -name b_l2

create_instance -block ROOT_BANK_CLKMUX -name u_gbox_root_bank_clkmux_hv_1 -logic_address 20 -parent GBOX_HV_40X2_VL
create_instance -block ROOT_BANK_CLKMUX -name u_gbox_root_bank_clkmux_hv_0 -logic_address 0 -parent GBOX_HV_40X2_VL
create_instance -block FCLK_MUX -name u_gbox_fclk_mux_hv_all -logic_address 40 -parent GBOX_HV_40X2_VL
create_instance -block GBOX_HV_40X2_VL -name inst_hv -parent b_l2
create_instance -block b_l2 -name inst_l2 -parent b_l1
create_instance -block ROOT_BANK_CLKMUX -name u_gbox

# Test cases for get_instance_names
test get_instance_names_at_device_level {Get all instance names at the device level} {
    get_instance_names
} {u_gbox}

test get_instance_names_at_block_level {Get instance names within block b_l1} {
    get_instance_names -block b_l1
} {inst_l2}

test get_instance_names_within_block {Get instance names within block b_l2} {
    get_instance_names -block b_l2
} {inst_hv}

# Test cases for IO bank operations
test set_and_get_io_bank {Set and get IO bank for an instance} {
    set_io_bank -inst b_l1.inst_l2.inst_hv.u_gbox_root_bank_clkmux_hv_0 -io_bank bank_99
    get_io_bank -inst b_l1.inst_l2.inst_hv.u_gbox_root_bank_clkmux_hv_0
} {bank_99}

# Test cases for physical address operations
test set_and_get_phy_address {Set and get physical address for an instance} {
    set_phy_address -inst b_l1.inst_l2.inst_hv.u_gbox_root_bank_clkmux_hv_0 -address 99
    get_phy_address -inst b_l1.inst_l2.inst_hv.u_gbox_root_bank_clkmux_hv_0
} {99}

# Test cases for logic location operations
test set_and_get_logic_location {Set and get logic location for an instance} {
    set_logic_location -inst b_l1.inst_l2.inst_hv.u_gbox_root_bank_clkmux_hv_0 -x 3 -y 6 -z 9
    get_logic_location -inst b_l1.inst_l2.inst_hv.u_gbox_root_bank_clkmux_hv_0
} {3 6 9}

# Test cases for logic address operations
test set_and_get_logic_address {Set and get logic address for an instance} {
    set_logic_address -inst b_l1.inst_l2.inst_hv.u_gbox_root_bank_clkmux_hv_0 -address 36
    get_logic_address -inst b_l1.inst_l2.inst_hv.u_gbox_root_bank_clkmux_hv_0
} {36}

cleanupTests
