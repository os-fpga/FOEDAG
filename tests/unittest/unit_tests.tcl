package require tcltest
namespace import -force ::tcltest::*
set script_directory [file dirname [info script]]

# Get a list of all test files (modify the pattern if needed)
set test_files [glob $script_directory/*/tcl_tests/unit_test_*.tcl]

foreach file $test_files {
    puts "Executing unit tests in $file"
    source $file
}
