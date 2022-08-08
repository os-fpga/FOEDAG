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
)

set (DISPLAY 0.0)
add_executable(unittest unittest_main.cpp ${CPP_LIST})

include_directories(${PROJECT_SOURCE_DIR}/../../src ${PROJECT_SOURCE_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/../../include/)

target_link_libraries(unittest PRIVATE Qt5::Widgets Qt5::Core Qt5::Gui Qt5::Xml qscintilla2_qt QConsole gtest gmock gtest_main foedag)

add_test(NAME unittest COMMAND unittest)
