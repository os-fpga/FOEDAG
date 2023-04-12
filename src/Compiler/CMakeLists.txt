# -*- mode:cmake -*-

# Copyright 2021 The Foedag team

# GPL License

# Copyright (c) 2021 The Open-Source FPGA Foundation

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.15)

project(compiler LANGUAGES CXX)

if (PRODUCTION_BUILD)
  message("Production Build type set to ON")
  set (PRODUCTION_BUILD_FLAG "-DPRODUCTION_BUILD=1")
endif(PRODUCTION_BUILD)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
else()
  set(CMAKE_CXX_FLAGS_DEBUG
  "${CMAKE_CXX_FLAGS_DEBUG} ${TCMALLOC_COMPILE_OPTIONS} -Werror -Wall -O0 -g ${MSYS_COMPILE_OPTIONS} ${MY_CXX_WARNING_FLAGS} ${MEM_SANITIZER_FLAGS} ${PRODUCTION_BUILD_FLAG}")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${PRODUCTION_BUILD_FLAG} -Werror -Wall")
endif()

include (../../cmake/cmake_qt.txt)
include (../../cmake/cmake_tcl.txt)

include_directories(${PROJECT_SOURCE_DIR}/../../src ${PROJECT_SOURCE_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/../../include/)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../lib)

set (SRC_CPP_LIST
  Compiler.cpp
  foedag_version_number.cpp
  Constraints.cpp
  CompilerOpenFPGA.cpp
  WorkerThread.cpp
  TaskTableView.cpp
  TaskModel.cpp
  Task.cpp
  TaskManager.cpp
  CompilerDefines.cpp
  Log.cpp
  Reports/AbstractReportManager.cpp
  Reports/DefaultTaskReport.cpp
  Reports/RoutingReportManager.cpp
  Reports/PackingReportManager.cpp
  Reports/PlacementReportManager.cpp
  Reports/TableReport.cpp
  Reports/TaskReportManagerRegistry.cpp
  Reports/SynthesisReportManager.cpp
  Reports/TimingAnalysisReportManager.cpp
  Reports/BitstreamReportManager.cpp
)

set (SRC_H_INSTALL_LIST
  Compiler.h
  Constraints.cpp
  CompilerOpenFPGA.h
  WorkerThread.h
  TaskTableView.h
  TaskModel.h
  Task.h
  TaskManager.h
  CompilerDefines.h
  Log.h
  Reports/AbstractReportManager.h
  Reports/DefaultTaskReport.h
  Reports/IDataReport.h
  Reports/ITaskReport.h
  Reports/ITaskReportManager.h
  Reports/RoutingReportManager.h
  Reports/PackingReportManager.h
  Reports/PlacementReportManager.h
  Reports/TableReport.h
  Reports/TaskReportManagerRegistry.h
  Reports/SynthesisReportManager.h
  Reports/TimingAnalysisReportManager.h
  Reports/BitstreamReportManager.h
  TaskGlobal.h
)

set (SRC_H_LIST
  ${SRC_H_INSTALL_LIST}
  TclInterpreterHandler.h
)

set (res_LIST
  compiler_resources.qrc
)

add_library(compiler STATIC
  ${SRC_CPP_LIST}
  ${SRC_H_LIST}
  ${res_LIST}
)

target_link_libraries(compiler PUBLIC Qt5::Widgets Qt5::Core Qt5::Gui Qt5::Xml)
target_compile_definitions(compiler PRIVATE COMPILER_LIBRARY)

install (
  FILES ${CMAKE_CURRENT_BINARY_DIR}/../../lib/$<TARGET_FILE_NAME:compiler>
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/foedag)

foreach(file ${SRC_H_INSTALL_LIST})
  install(
      FILES ${PROJECT_SOURCE_DIR}/../Compiler/${file}
      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/foedag/Compiler)
endforeach()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../bin)

add_executable(compiler_bin
	${PROJECT_SOURCE_DIR}/../Compiler/Test/compiler_main.cpp)
target_link_libraries(compiler_bin foedag tcl_stubb tcl_static zlib compiler ipgenerate designquery simulation)
set_target_properties(compiler_bin PROPERTIES OUTPUT_NAME compiler_test)

