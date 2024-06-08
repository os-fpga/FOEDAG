package require tcltest ; # Make sure tcltest is loaded first
namespace import -force ::tcltest::*

# Test setup: Define the block and ports
define_block -name test_block
define_ports -block test_block -in a1 b1 c1 -out aa1 bb1 cc1

# Test cases for get_port_list
test get_all_ports {Get all ports from block} {
    get_port_list -block test_block
} {cc1 bb1 aa1 c1 b1 a1}

test get_input_ports {Get input ports from block} {
    get_port_list -block test_block -dir in
} {c1 b1 a1}

test get_output_ports {Get output ports from block} {
    get_port_list -block test_block -dir out
} {cc1 bb1 aa1}

# Error handling tests
test missing_block_argument {Missing block argument} {
    catch {get_port_list} err
    set err
} {Insufficient arguments passed to get_port_list.}

test invalid_block_name {Invalid block name} {
    catch {get_port_list -block non_existent_block} err
    set err
} {Block non_existent_block does not exist for get_port_list.}

cleanupTests
