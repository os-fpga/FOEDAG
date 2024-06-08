package require tcltest  ; # Make sure tcltest is loaded first
namespace import -force ::tcltest::*

# Define the block and its attributes
define_block -name Block_TOP

define_attr -block Block_TOP -name RATE         -addr 0  -width 4 -enumname Block_ATT_0  -enum {Three 3} {Four 4} {Five 5} {Six 6} {Seven 7} {Eight 8} {Night 9} {Ten 10}
define_attr -block Block_TOP -name MASTER_SLAVE -addr 4  -width 1 -enumname Block_ATT_1  -enum {Slave 0} {Master 1}
define_attr -block Block_TOP -name PEER_IS_ON   -addr 5  -width 1 -enumname Block_ATT_2  -enum {PEER_off 0} {PEER_ON 1}
define_attr -block Block_TOP -name TX_CLOCK_IO  -addr 6  -width 1 -enumname Block_ATT_3  -enum {TX_normal_IO 0} {TX_clock_IO 1}
define_attr -block Block_TOP -name TX_DDR_MODE  -addr 7  -width 2 -enumname Block_ATT_4  -enum {TX_direct 0} {TX_ddr 1} {TX_sdr 2}
define_attr -block Block_TOP -name TX_BYPASS    -addr 9  -width 1 -enumname Block_ATT_5  -enum {TX_gear_on 0} {TX_bypass 1}
define_attr -block Block_TOP -name TX_CLK_PHASE -addr 10 -width 2 -enumname Block_ATT_6  -enum {TX_phase_0 0} {TX_phase_90 1} {TX_phase_180 2} {TX_phase_270 3}
define_attr -block Block_TOP -name TX_DLY       -addr 12 -width 6

# Unit tests for get_attributes command

test get_all_attributes {Get all attributes from block} {
    get_attributes -block Block_TOP
} {TX_DLY TX_BYPASS TX_DDR_MODE PEER_IS_ON MASTER_SLAVE TX_CLK_PHASE TX_CLOCK_IO RATE}

test missing_block_argument {Missing block argument} {
    catch {get_attributes} err
    set err
} {}

test invalid_block_name {Invalid block name} {
    catch {get_attributes -block non_existent_block} err
    set err
} {Block non_existent_block does not exist for get_attributes.}

cleanupTests
