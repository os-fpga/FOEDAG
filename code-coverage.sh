#!/usr/bin/env bash
set -e # Exit immediately if a pipeline (command or list of commands) returns a non-zero status

BUILD_DIR="dbuild"
COVERAGE_DIR="$BUILD_DIR/code-coverage"
CI_PROJECT_DIR=$1
CPU_CORES=$(grep --count ^processor /proc/cpuinfo)
GCOVR_VER=$(gcovr --version | grep -Po 'gcovr ([+-]?[[0-9]*[.]]?[0-9]+)' | awk -F' ' '{print $2}')

if (( $(echo "$GCOVR_VER < 6.0" |bc -l) )); then
    echo "gcovr version must be >=6.0 but $GCOVR_VER is provided"
    exit 1
fi

# Clean the previous build
rm -rf $BUILD_DIR

# Build the project
cmake . -B$BUILD_DIR -DCMAKE_BUILD_TYPE=Debug -DCODE_COVERAGE:BOOL=ON
make -C $BUILD_DIR --no-print-directory -j $CPU_CORES

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

mkdir -p $COVERAGE_DIR
gcovr -r . -f src* -s --xml -o $COVERAGE_DIR/index.xml --exclude-unreachable-branches --exclude-throw-branches\
    -e "src/Compiler/Test/*" \
    -e "src/Console/Test/*" \
    -e "src/DesignRuns/Test/*" \
    -e "src/IPGenerate/Test/*" \
    -e "src/IpConfigurator/Test/*" \
    -e "src/NewFile/Test/*" \
    -e "src/PinAssignment/Test/*" \
    -e "src/ProjNavigator/Test/*" \
    -e "src/TextEditor/Test/*" \
    -e "src/NewProject/Main/*"
