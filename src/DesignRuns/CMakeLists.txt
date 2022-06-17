cmake_minimum_required(VERSION 3.15)

project(designruns LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
else()
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Werror")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Werror")
endif()

include (../../cmake/cmake_qt.txt)
include (../../cmake/cmake_tcl.txt)

include_directories(
  ${PROJECT_SOURCE_DIR}/../../src
  ${CMAKE_CURRENT_BINARY_DIR}/../../include/)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../lib)

set (SRC_CPP_LIST
 runs_form.cpp
 create_runs_dialog.cpp
 create_runs_form.cpp
 select_design_type_form.cpp
 runs_summary_form.cpp
 runs_grid.cpp
 runs_grid_delegate.cpp
 device_planner_dialog.cpp)

set (SRC_H_LIST
 runs_form.h
 create_runs_dialog.h
 create_runs_form.h
 select_design_type_form.h
 runs_summary_form.h
 runs_grid.h
 runs_grid_delegate.h
 device_planner_dialog.h)

set (SRC_UI_LIST
 create_runs_dialog.ui
 create_runs_form.ui
 device_planner_dialog.ui
 select_design_type_form.ui
 runs_summary_form.ui)

add_library(designruns STATIC
  ${SRC_CPP_LIST}
  ${SRC_H_LIST}
  ${SRC_UI_LIST}
)

target_link_libraries(designruns  PUBLIC Qt5::Widgets Qt5::Core Qt5::Gui)
target_compile_definitions(designruns PRIVATE DESIGNRUNS_LIBRARY)

set(DESIGNRUNS_STATIC_LIB libdesignruns.a)
if (MSVC)
 set(DESIGNRUNS_STATIC_LIB designruns.lib)
endif()

install (
 FILES ${CMAKE_CURRENT_BINARY_DIR}/../../lib/${DESIGNRUNS_STATIC_LIB}
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/foedag)

install(
    FILES ${PROJECT_SOURCE_DIR}/../DesignRuns/runs_form.h
    FILES ${PROJECT_SOURCE_DIR}/../DesignRuns/create_runs_dialog.h
    FILES ${PROJECT_SOURCE_DIR}/../DesignRuns/create_runs_form.h
    FILES ${PROJECT_SOURCE_DIR}/../DesignRuns/runs_grid.h
    FILES ${PROJECT_SOURCE_DIR}/../DesignRuns/runs_grid_delegate.h
    FILES ${PROJECT_SOURCE_DIR}/../DesignRuns/device_planner_dialog.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/foedag/DesignRuns)
  
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../bin)

add_executable(designruns_bin
 ${PROJECT_SOURCE_DIR}/../DesignRuns/Test/design_runs_main.cpp
 ${PROJECT_SOURCE_DIR}/../DesignRuns/Test/design_runs_commands.cpp)

target_link_libraries(designruns_bin foedag tcl_stubb tcl_static zlib tclutils)
set_target_properties(designruns_bin PROPERTIES OUTPUT_NAME designruns)

