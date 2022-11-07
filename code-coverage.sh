#!/usr/bin/env bash
set -e # Exit immediately if a pipeline (command or list of commands) returns a non-zero status

BUILD_DIR="dbuild"
COVERAGE_DIR="$BUILD_DIR/code-coverage"
CI_PROJECT_DIR=$1
CPU_CORES=$(grep --count ^processor /proc/cpuinfo)

# Clean the previous build
rm -rf $BUILD_DIR

# Build the project
cmake . -B$BUILD_DIR -DCMAKE_BUILD_TYPE=Debug -DCODE_COVERAGE:BOOL=ON
make -C $BUILD_DIR --no-print-directory -j $CPU_CORES

mkdir -p $COVERAGE_DIR

# Collect initial lcov data
lcov --capture --initial --base-directory . --directory $BUILD_DIR --no-external \
    --output-file $COVERAGE_DIR/app_base.info

# Run all tests
pushd $BUILD_DIR/tests/unittest
if [ -f /.dockerenv ]; then
    export DISPLAY=:99
fi

set +e # do not exit in case some tests failed. Need to collect results
./unittest -platform offscreen --gtest_output="xml:junitresults.xml"
test_results=$?
set -e

if [ "$test_results" -ne "0" ]; then
        exit $test_results
fi
popd

# Collect lcov data after the tests
lcov --rc lcov_branch_coverage=1 --capture --base-directory . --directory $BUILD_DIR \
    --no-external --output-file $COVERAGE_DIR/app_test.info

# Combine info from two files
lcov --rc lcov_branch_coverage=1 -a $COVERAGE_DIR/app_base.info -a \
    $COVERAGE_DIR/app_test.info --output-file $COVERAGE_DIR/app.info

# Remove unneeded folders from info file
lcov --rc lcov_branch_coverage=1 --remove $COVERAGE_DIR/app.info \
    "$PWD/$BUILD_DIR/*" \
    "$PWD/third_party/*" \
    "$PWD/tests/*" \
    "$PWD/src/Compiler/Test/*" \
    "$PWD/src/Console/Test/*" \
    "$PWD/src/DesignRuns/Test/*" \
    "$PWD/src/IPGenerate/Test/*" \
    "$PWD/src/IpConfigurator/Test/*" \
    "$PWD/src/NewFile/Test/*" \
    "$PWD/src/PinAssignment/Test/*" \
    "$PWD/src/ProjNavigator/Test/*" \
    "$PWD/src/TextEditor/Test/*" \
    "$PWD/src/NewProject/Main/*" \
    --output-file $COVERAGE_DIR/app_filtered.info

# Generate html report
genhtml $COVERAGE_DIR/app_filtered.info --output-directory \
    $COVERAGE_DIR/report --branch-coverage --show-details

echo "*** DONE *** Report saved to $COVERAGE_DIR/report"
