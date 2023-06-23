cmake_minimum_required(VERSION 3.15)

project(unittest LANGUAGES CXX)

enable_testing()
include(GoogleTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_definitions(-DGTEST_LANGUAGE_CXX17)

if(MSVC)
  # Microsoft reports the value of __cplusplus wrong and gmock/gtest pulls in the
  # string_view implementation based on it's value. Microsoft's solution is to
  # provide additional flags to make the value correct. More info can be found here -
  #
  #   https://docs.microsoft.com/en-us/cpp/build/reference/zc-cplusplus?view=msvc-160
  #   https://devblogs.microsoft.com/cppblog/msvc-now-correctly-reports-__cplusplus/
  target_compile_options(gmock PRIVATE /Zc:__cplusplus)
  target_compile_options(gmock_main PRIVATE /Zc:__cplusplus)
  target_compile_options(gtest PRIVATE /Zc:__cplusplus)
  target_compile_options(gtest_main PRIVATE /Zc:__cplusplus)
endif()

set (CPP_LIST
    Tcl/TclInterpreter_test.cpp
    Command/Command_test.cpp
    Utils/StringUtils_test.cpp
    NewProject/ProjectManager_test.cpp
    PinAssignment/BufferedComboBox_test.cpp
#    PinAssignment/PinAssignmentCreator_test.cpp // TODO @volodymyrk RG-181
#    PinAssignment/PinsBaseModel_test.cpp // TODO @volodymyrk RG-181
    PinAssignment/PortsLoader_test.cpp
#    PinAssignment/PackagePinsLoader_test.cpp // TODO @volodymyrk RG-181
    Settings/Settings_test.cpp
    IPGenerator/IPGenerator_test.cpp
    NewProject/source_grid_test.cpp
    Utils/sequential_map_test.cpp
    Utils/QtUtils_test.cpp
    PinAssignment/TestLoader.cpp
    PinAssignment/TestPortsLoader.cpp
    Compiler/CompilerDefines_test.cpp
    PinAssignment/PortsModel_test.cpp
    PinAssignment/PinAssignmentBaseView_test.cpp
    Simulation/Simulation_test.cpp
    Utils/FileUtils_test.cpp
)
set (H_LIST
    PinAssignment/TestLoader.h
    PinAssignment/TestPortsLoader.h
)

add_executable(unittest unittest_main.cpp ${CPP_LIST} ${H_LIST} resources.qrc)

include_directories(${PROJECT_SOURCE_DIR}/../../src ${PROJECT_SOURCE_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/../../include/)

target_link_libraries(unittest PRIVATE
  gtest
  gmock
  gtest_main
  pinassignment
  newproject
  foedag
  foedagcore)

add_test(NAME unittest COMMAND unittest)
