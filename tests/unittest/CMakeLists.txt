cmake_minimum_required(VERSION 3.15)

project(unittest LANGUAGES CXX)

enable_testing()
include(GoogleTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_definitions(-DGTEST_LANGUAGE_CXX17)

# Python
if (MINGW OR (NOT MSVC AND WIN32))
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/")
  find_package(Python3 REQUIRED)
else()
  find_package(Python3 3.3 REQUIRED COMPONENTS Interpreter Development)
endif()
message(STATUS "Python3_LIBRARIES (Unit) = ${Python3_LIBRARIES}")
message(STATUS "Python3_INCLUDE_DIRS (Unit) = ${Python3_INCLUDE_DIRS}")
message(STATUS "Python3_RUNTIME_LIBRARY_DIRS (Unit) = ${Python3_RUNTIME_LIBRARY_DIRS}")

find_package(Qt6 COMPONENTS Test REQUIRED)
if (NOT Qt6Test_FOUND)
    message(STATUS "Failed to find Qt6Test required (on debian/ubuntu try 'sudo apt install qt6-base-dev')")
endif()

if(MSVC)
  # Microsoft reports the value of __cplusplus wrong and gmock/gtest pulls in the
  # string_view implementation based on it's value. Microsoft's solution is to
  # provide additional flags to make the value correct. More info can be found here -
  #
  # https://docs.microsoft.com/en-us/cpp/build/reference/zc-cplusplus?view=msvc-160
  # https://devblogs.microsoft.com/cppblog/msvc-now-correctly-reports-__cplusplus/
  target_compile_options(gmock PRIVATE /Zc:__cplusplus)
  target_compile_options(gmock_main PRIVATE /Zc:__cplusplus)
  target_compile_options(gtest PRIVATE /Zc:__cplusplus)
  target_compile_options(gtest_main PRIVATE /Zc:__cplusplus)
  add_compile_options(/bigobj)
endif()

if(MINGW)
  add_compile_options(-Wa,-mbig-obj)
endif()

set(CPP_LIST
  CompilerTCLCommonCode/compiler_tcl_infra_common.cpp
  
  Tcl/TclInterpreter_test.cpp
  Command/Command_test.cpp
  Utils/StringUtils_test.cpp
  NewProject/ProjectManager_test.cpp
  PinAssignment/BufferedComboBox_test.cpp

  # PinAssignment/PinAssignmentCreator_test.cpp // TODO @volodymyrk RG-181
  # PinAssignment/PinsBaseModel_test.cpp // TODO @volodymyrk RG-181
  PinAssignment/PortsLoader_test.cpp

  # PinAssignment/PackagePinsLoader_test.cpp // TODO @volodymyrk RG-181
  Settings/Settings_test.cpp
  IPGenerator/IPGenerator_test.cpp
  NewProject/source_grid_test.cpp
  Utils/sequential_map_test.cpp
  Utils/QtUtils_test.cpp
  PinAssignment/TestLoader.cpp
  PinAssignment/TestPortsLoader.cpp
  Constraints/Constraints_test.cpp
  Compiler/CompilerDefines_test.cpp
  Compiler/Compiler_test.cpp
  PinAssignment/PortsModel_test.cpp
  PinAssignment/PinAssignmentBaseView_test.cpp
  Simulation/Simulation_test.cpp
  Utils/FileUtils_test.cpp
  CFGCommon/CFGCommon_test.cpp
  CFGCommon/CFGArg_test.cpp
  CFGCompiler/CFGCompiler_test.cpp
  ModelConfig/ModelConfig_test.cpp
  ModelConfig/ModelConfig_IO_test.cpp
  CFGProgrammer/CFGProgrammer_test.cpp
  MainWindow/PerfomanceTracker_test.cpp
  MainWindow/ProjectFileComponent_test.cpp
  DeviceModeling/rs_expression_test.cpp
  DeviceModeling/rs_expression_evaluator_test.cpp
  DeviceModeling/rs_parameter_type_test.cpp
  DeviceModeling/rs_parameter_test.cpp
  DeviceModeling/device_signal_test.cpp
  DeviceModeling/device_net_test.cpp
  DeviceModeling/device_port_test.cpp
  DeviceModeling/device_instance_test.cpp
  DeviceModeling/device_test.cpp
  DeviceModeling/device_modeler_test.cpp
  Compiler/TaskManager_test.cpp
  ProgrammerGui/SummaryProgressBar_test.cpp
  ProjNavigator/HierarchyView_test.cpp
  Settings/CompilerSettings_test.cpp
  Utils/ArgumentsMap_test.cpp
  rapidgpt/rapidgpt_test.cpp
  rapidgpt/ChatWidget_test.cpp
)

if (USE_IPA)
  set(CPP_LIST ${CPP_LIST}
    InteractivePathAnalysis/ZlibUtils_test.cpp
    InteractivePathAnalysis/ConvertUtils_test.cpp
    InteractivePathAnalysis/TelegramParser_test.cpp
    InteractivePathAnalysis/TelegramBuffer_test.cpp
    InteractivePathAnalysis/NCriticalPathModel_test.cpp
  )
endif()

set(H_LIST
  CompilerTCLCommonCode/compiler_tcl_infra_common.h
  PinAssignment/TestLoader.h
  PinAssignment/TestPortsLoader.h
)

add_executable(unittest unittest_main.cpp ${CPP_LIST} ${H_LIST} resources.qrc)

include_directories(
  ${PROJECT_SOURCE_DIR}/../../src 
  ${PROJECT_SOURCE_DIR}/.. 
  ${PROJECT_SOURCE_DIR}/CompilerTCLCommonCode
  ${CMAKE_CURRENT_BINARY_DIR}/../../include/ 
  ${CMAKE_CURRENT_BINARY_DIR}/../../src/Configuration/CFGCommon
  ${Python3_INCLUDE_DIRS}
)

target_link_libraries(unittest PRIVATE
  gtest
  gmock
  gtest_main
  pinassignment
  newproject
  foedag
  foedagcore
  cfgcommon
  modelconfig
  programmer
  programmer-gui
  Qt6::Test
  ${Python3_LIBRARIES})

add_test(NAME unittest COMMAND unittest)

if(MSVC)
  set(VCPKG_LIBUSB_DIR "$ENV{VCPKG_INSTALLATION_ROOT}/packages/libusb_x64-windows")
  file(TO_CMAKE_PATH "${VCPKG_LIBUSB_DIR}" VCPKG_LIBUSB_DIR)
  set(libusb_dll "${VCPKG_LIBUSB_DIR}/bin/libusb-1.0.dll")
  file(COPY ${libusb_dll} DESTINATION  ${CMAKE_CURRENT_BINARY_DIR})
endif()
