package require tcltest ; # Make sure tcltest is loaded first
namespace import -force ::tcltest::*

# Test setup: Define the blocks and create instances
device_name 09-Device

define_block -name High_Block
define_block -name ROOT_BANK_CLKMUX
define_block -name FCLK_MUX
define_block -name b_l1
define_block -name b_l2

create_instance -block ROOT_BANK_CLKMUX -name u_Block_root_bank_clkmux_hv_1 -logic_address 20 -parent High_Block
create_instance -block ROOT_BANK_CLKMUX -name u_Block_root_bank_clkmux_hv_0 -logic_address 0 -parent High_Block
create_instance -block FCLK_MUX -name u_Block_fclk_mux_hv_all -logic_address 40 -parent High_Block
create_instance -block High_Block -name inst_hv -parent b_l2
create_instance -block b_l2 -name inst_l2 -parent b_l1
create_instance -block ROOT_BANK_CLKMUX -name u_gbox
create_instance -block MUX2X1 -name mx21

# Test cases for get_instance_names
test get_all_instance_names_device_level {Get all instance names at device level} {
    get_instance_names
} {mx21 u_gbox}

test get_instance_names_block_b_l1 {Get instance names from block b_l1} {
    get_instance_names -block b_l1
} {inst_l2}

test get_instance_names_block_b_l2 {Get instance names from block b_l2} {
    get_instance_names -block b_l2
} {inst_hv}

test get_instance_names_block_High_Block {Get instance names from block High_Block} {
    get_instance_names -block High_Block
} {u_Block_fclk_mux_hv_all u_Block_root_bank_clkmux_hv_0 u_Block_root_bank_clkmux_hv_1}

# Test cases for get_instance_block_type
test get_instance_block_type_u_gbox {Get block type of instance u_gbox} {
    get_instance_block_type -name u_gbox
} {ROOT_BANK_CLKMUX}

test get_instance_block_type_u_Block_fclk_mux_hv_all {Get block type of instance High_Block.u_Block_fclk_mux_hv_all} {
    get_instance_block_type -name High_Block.u_Block_fclk_mux_hv_all
} {FCLK_MUX}

test get_instance_block_type_u_Block_root_bank_clkmux_hv_0 {Get block type of instance High_Block.u_Block_root_bank_clkmux_hv_0} {
    get_instance_block_type -name High_Block.u_Block_root_bank_clkmux_hv_0
} {ROOT_BANK_CLKMUX}

test get_instance_block_type_nested_instance {Get block type of nested instance b_l1.inst_l2.inst_hv.u_Block_root_bank_clkmux_hv_0} {
    get_instance_block_type -name b_l1.inst_l2.inst_hv.u_Block_root_bank_clkmux_hv_0
} {ROOT_BANK_CLKMUX}

# Error handling tests
test missing_instance_name_argument {Missing instance name argument} {
    catch {get_instance_block_type} err
    set err
} {Need at least 2 arguments for command get_instance_block_type}

test invalid_instance_name {Invalid instance name} {
    catch {get_instance_block_type -name non_existent_instance} err
    set err
} {Could not find instance non_existent_instance}

cleanupTests
