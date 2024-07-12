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

project(console LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
else()
  set(CMAKE_CXX_FLAGS_DEBUG
  "${CMAKE_CXX_FLAGS_DEBUG} ${TCMALLOC_COMPILE_OPTIONS} -Werror -Wall -O0 -g ${MSYS_COMPILE_OPTIONS} ${MY_CXX_WARNING_FLAGS} ${MEM_SANITIZER_FLAGS}")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Werror")
endif()

include (../../cmake/cmake_qt.txt)
include (../../cmake/cmake_tcl.txt)

include_directories(${PROJECT_SOURCE_DIR}/../../src ${PROJECT_SOURCE_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/../../include/)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../lib)

set (SRC_CPP_LIST
  PinAssignmentCreator.cpp
  PackagePinsView.cpp
  PackagePinsModel.cpp
  PinsBaseModel.cpp
  PortsView.cpp
  PortsModel.cpp
  PackagePinsLoader.cpp
  QLPackagePinsLoader.cpp
  PortsLoader.cpp
  BufferedComboBox.cpp
  PinAssignmentBaseView.cpp
  ComboBox.cpp
)

set (H_INSTALL_LIST
  PinAssignmentCreator.h
)

set (SRC_H_LIST
  ${H_INSTALL_LIST}
  PackagePinsView.h
  PackagePinsModel.h
  PinsBaseModel.h
  PortsView.h
  PortsModel.h
  PackagePinsLoader.h
  QLPackagePinsLoader.h
  PortsLoader.h
  BufferedComboBox.h
  PinAssignmentBaseView.h
  ComboBox.h
)

set (SRC_UI_LIST
)

add_library(pinassignment STATIC
  ${SRC_CPP_LIST}
  ${SRC_H_LIST}
  ${SRC_UI_LIST}
)

target_link_libraries(pinassignment PUBLIC Qt5::Widgets Qt5::Core Qt5::Gui tcl_stubb tcl_static zlib)
target_compile_definitions(pinassignment PRIVATE pinassignment_LIBRARY)

set (HEADER_INSTALL_LIST "")
foreach(HEADER ${H_INSTALL_LIST})
  list(APPEND HEADER_INSTALL_LIST ${PROJECT_SOURCE_DIR}/../PinAssignment/${HEADER})
endforeach()

install (
  FILES ${CMAKE_CURRENT_BINARY_DIR}/../../lib/$<TARGET_FILE_NAME:pinassignment>
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/foedag)

install(
   FILES ${HEADER_INSTALL_LIST}
   DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/foedag/PinAssignment)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../bin)

add_executable(pinassignment_bin
  ${PROJECT_SOURCE_DIR}/../PinAssignment/Test/PinAssignment_main.cpp
  ${PROJECT_SOURCE_DIR}/../PinAssignment/Test/PinAssignmentCommands.cpp)
target_link_libraries(pinassignment_bin foedag foedagcore tcl_stubb tcl_static
  zlib tclutils pinassignment)
set_target_properties(pinassignment_bin PROPERTIES OUTPUT_NAME pinassignment)
