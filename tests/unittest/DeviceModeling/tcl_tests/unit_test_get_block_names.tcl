package require tcltest ; # Make sure tcltest is loaded first
namespace import -force ::tcltest::*

# Test setup: Define the initial blocks and devices
device_name test_device

define_block -name Block_TOP1
define_block -name Block_TOP2
define_block -name Block_TOP3

test get_all_blocks_test_device {Get all blocks from test_device} {
    get_block_names
} {Block_TOP1 MUX32X1 MUX16X1 Block_TOP3 MUX8X1 Block_TOP2 MUX4X1 MUX2X1}

# Set a different device and define a block there
device_name other_test_device

define_block -name Block_TIP

test get_all_blocks_other_test_device {Get all blocks from other_test_device} {
    get_block_names
} {Block_TIP MUX32X1 MUX16X1 MUX8X1 MUX4X1 MUX2X1}

test get_all_blocks_test_device_specific {Get all blocks from test_device specifically} {
    get_block_names -device test_device
} {Block_TOP1 MUX32X1 MUX16X1 Block_TOP3 MUX8X1 Block_TOP2 MUX4X1 MUX2X1}

# Test cases for error handling
test missing_device_argument {Missing device argument} {
    catch {get_block_names -device} err
    set err
} {Block_TIP MUX32X1 MUX16X1 MUX8X1 MUX4X1 MUX2X1}

test invalid_device_name {Invalid device name} {
    catch {get_block_names -device non_existent_device} err
    set err
} {Could not find device named non_existent_device}

cleanupTests
