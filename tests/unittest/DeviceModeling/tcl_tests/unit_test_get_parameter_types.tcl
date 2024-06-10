package require tcltest ; # Make sure tcltest is loaded first
namespace import -force ::tcltest::*

# Test setup: Define the block and parameter types
define_block -name GBOX_TOP

define_param_type -block GBOX_TOP -name int_t1 -base_type int -default 9 -width 6
define_param_type -block GBOX_TOP -name int_t2 -base_type int -default 18 -width 6
define_param_type -block GBOX_TOP -name double_t1 -base_type double -default 18.9
define_param_type -block GBOX_TOP -name string_t1 -base_type string -default "default_val"

# Test cases for get_parameter_types
test get_all_parameter_types {Get all parameter types from block} {
    get_parameter_types -block GBOX_TOP
} {int_t2 int_t1 double_t1 string_t1}

test get_string_parameter_types {Get string parameter types from block} {
    get_parameter_types -block GBOX_TOP -base_type string
} {string_t1}

test get_double_parameter_types {Get double parameter types from block} {
    get_parameter_types -block GBOX_TOP -base_type double
} {double_t1}

test get_int_parameter_types {Get int parameter types from block} {
    get_parameter_types -block GBOX_TOP -base_type int
} {int_t2 int_t1}

# Error handling tests
test missing_block_argument {Missing block argument} {
    catch {get_parameter_types} err
    set err
} {int double string}

test invalid_block_name {Invalid block name} {
    catch {get_parameter_types -block non_existent_block} err
    set err
} {Block non_existent_block does not exist for get_parameter_types.}

cleanupTests
