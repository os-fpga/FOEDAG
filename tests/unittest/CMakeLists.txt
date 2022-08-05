cmake_minimum_required(VERSION 3.15)

project(unittest LANGUAGES CXX)

enable_testing()
include(GoogleTest)

include (../../cmake/cmake_qt.txt)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_definitions(-DGTEST_LANGUAGE_CXX17)

# All unit-tests are registered with this custom target as dependency, so
# just `make UnitTests` will build them all.
add_custom_target(UnitTests)

add_executable(unittest unittest_main.cpp
    Tcl/TclInterpreter_test.cpp
    Command/Command_test.cpp
    Utils/StringUtils_test.cpp)

gtest_discover_tests(unittest
    Tcl/TclInterpreter_test.cpp
    Command/Command_test.cpp
    Utils/StringUtils_test.cpp
    AUTO)

# Link all unit tests with 'make UnitTests' command
add_dependencies(UnitTests unittest)

include_directories(${PROJECT_SOURCE_DIR}/../../src ${PROJECT_SOURCE_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/../../include/)

target_link_libraries(unittest PRIVATE Qt5::Widgets Qt5::Core Qt5::Gui Qt5::Xml gtest gmock gtest_main foedag)
