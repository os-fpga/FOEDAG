package require tcltest ;# Make sure tcltest is loaded first
namespace import -force ::tcltest::*

# Define the block Top_Block
define_block -name Top_Block

# Define attributes for the block Top_Block
define_attr -block Top_Block -name RATE -addr 0 -width 4 -enumname Block_ATT_0 -enum {Three 3} {Four 4} {Five 5} {Six 6} {Seven 7} {Eight 8} {Night 9} {Ten 10}
define_attr -block Top_Block -name MASTER_SLAVE -addr 4 -width 1 -enumname Block_ATT_1 -enum {Slave 0} {Master 1}
define_attr -block Top_Block -name PEER_IS_ON -addr 5 -width 1 -enumname Block_ATT_2 -enum {PEER_off 0} {PEER_ON 1}
define_attr -block Top_Block -name TX_CLOCK_IO -addr 6 -width 1 -enumname Block_ATT_3 -enum {TX_normal_IO 0} {TX_clock_IO 1}
define_attr -block Top_Block -name TX_DDR_MODE -addr 7 -width 2 -enumname Block_ATT_4 -enum {TX_direct 0} {TX_ddr 1} {TX_sdr 2}
define_attr -block Top_Block -name TX_BYPASS -addr 9 -width 1 -enumname Block_ATT_5 -enum {TX_gear_on 0} {TX_bypass 1}
define_attr -block Top_Block -name TX_CLK_PHASE -addr 10 -width 2 -enumname Block_ATT_6 -enum {TX_phase_0 0} {TX_phase_90 1} {TX_phase_180 2} {TX_phase_270 3}
define_attr -block Top_Block -name TX_DLY -addr 12 -width 6
define_attr -block Top_Block -name RX_DDR_MODE -addr 18 -width 2 -enumname Block_ATT_8 -enum {RX_direct 0} {RX_ddr 1} {RX_sdr 2}
define_attr -block Top_Block -name RX_BYPASS -addr 20 -width 1 -enumname Block_ATT_9 -enum {RX_gear_on 0} {RX_bypass 1}
define_attr -block Top_Block -name RX_DLY -addr 21 -width 6
define_attr -block Top_Block -name RX_DPA_MODE -addr 27 -width 2 -enumname Block_ATT_11 -enum {RX_dpa_off 0} {RX_resv 1} {RX_dpa 2} {RX_cdr 3}
define_attr -block Top_Block -name RX_MIPI_MODE -addr 29 -width 1 -enumname Block_ATT_12 -enum {RX_mipi_off 0} {RX_mipi_on 1}
define_attr -block Top_Block -name TX_MODE -addr 30 -width 1 -enumname Block_ATT_13 -enum {TX_disable 0} {TX_enable 1}
define_attr -block Top_Block -name RX_MODE -addr 31 -width 1 -enumname Block_ATT_14 -enum {RX_disable 0} {RX_enable 1}
define_attr -block Top_Block -name RX_CLOCK_IO -addr 32 -width 1 -enumname Block_ATT_15 -enum {RX_normal_IO 0} {RX_clock_IO 1}
define_attr -block Top_Block -name DFEN -addr 33 -width 1 -enumname Block_ATT_16 -enum {SingleEnded 0} {Differential 1}
define_attr -block Top_Block -name SR -addr 34 -width 1 -enumname Block_ATT_17 -enum {SR_disable 0} {SR_enable 1}
define_attr -block Top_Block -name PE -addr 35 -width 1 -enumname Block_ATT_18 -enum {PE_disable 0} {PE_enable 1}
define_attr -block Top_Block -name PUD -addr 36 -width 1 -enumname Block_ATT_19 -enum {PUD_disable 0} {PUD_enable 1}
define_attr -block Top_Block -name DFODTEN -addr 37 -width 1 -enumname Block_ATT_20 -enum {DF_odt_disable 0} {DF_odt_enable 1}
define_attr -block Top_Block -name MC -addr 38 -width 4

# Define constraints for the block Top_Block
define_constraint -block Top_Block -constraint {(DFEN == SingleEnded) -> {RATE inside {[Three:Ten]}}}
define_constraint -block Top_Block -constraint {(DFEN == Differential) -> {RATE inside {[Three:Ten]}}}
define_constraint -block Top_Block -constraint {(DFEN == Differential) -> (PEER_IS_ON == PEER_off)}
define_constraint -block Top_Block -constraint {(PEER_IS_ON == PEER_ON) -> {RATE inside {[Three:Five]}}}
define_constraint -block Top_Block -constraint {(RX_MIPI_MODE == RX_mipi_on) -> (DFEN == Differential)}
define_constraint -block Top_Block -constraint {(TX_DDR_MODE == TX_direct) -> (TX_BYPASS == TX_bypass)} -name my_funny_constraint
define_constraint -block Top_Block -constraint {(RX_DDR_MODE == RX_direct) -> (RX_BYPASS == RX_bypass)}

# Unit tests for constraints
test get_constraint_names {Get all constraint names from block} {
    get_constraint_names -block Top_Block
} {Top_Block_constraint_6 my_funny_constraint Top_Block_constraint_4 Top_Block_constraint_3 Top_Block_constraint_2 Top_Block_constraint_1 Top_Block_constraint_0}

test get_constraint_by_name_1 {Get constraint by name Top_Block_constraint_6} {
    get_constraint_by_name -block Top_Block -name Top_Block_constraint_6
} {(RX_DDR_MODE == RX_direct) -> (RX_BYPASS == RX_bypass)}

test get_constraint_by_name_2 {Get constraint by name my_funny_constraint} {
    get_constraint_by_name -block Top_Block -name my_funny_constraint
} {(TX_DDR_MODE == TX_direct) -> (TX_BYPASS == TX_bypass)}

# Error handling tests
test missing_block_argument {Missing block argument for get_constraint_names} {
    catch {get_constraint_names} err
    set err
} {}

test invalid_block_name {Invalid block name for get_constraint_names} {
    catch {get_constraint_names -block non_existent_block} err
    set err
} {Block non_existent_block does not exist for get_constraint_names.}

cleanupTests
