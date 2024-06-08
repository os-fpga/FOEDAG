package require tcltest
namespace import -force ::tcltest::*


# Create test data
define_block -name test_block

define_param_type -block test_block -name int_t1 -base_type int
define_param_type -block test_block -name int_t2 -base_type int
define_param -block test_block -name int_par1 -type int_t1
define_param -block test_block -name int_par2 -type int_t2
define_param -block test_block -name int_par3 -type int_t1

define_param_type -block test_block -name double_t1 -base_type double
define_param -block test_block -name double_par1 -type double_t1

define_param_type -block test_block -name string_t1 -base_type string
define_param -block test_block -name string_par1 -type string_t1

# --- Test Cases ---
test get_all_parameters {Get all parameters} {
    get_parameters -block test_block
} {int_par3 int_par2 int_par1 double_par1 string_par1}

test get_string_parameters {Get string parameters} {
    get_parameters -block test_block -base_type string
} {string_par1}

test get_double_parameters {Get double parameters} {
    get_parameters -block test_block -base_type double
} {double_par1}

test get_int_parameters {Get integer parameters} {
    get_parameters -block test_block -base_type int
} {int_par3 int_par2 int_par1}

test non_existent_block {Non-existent block} {
    catch {get_parameters -block not_a_block} err
    set err
} {}

test missing_block_argument {Missing block argument} {
    catch {get_parameters} err
    set err
} {}

# --- Run Tests ---
cleanupTests
