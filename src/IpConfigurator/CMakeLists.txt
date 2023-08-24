# -*- mode:cmake -*-

# Copyright 2022 The Foedag team

# GPL License

# Copyright (c) 2022 The Open-Source FPGA Foundation

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

project(ipconfigurator LANGUAGES CXX)

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

include_directories(
  ${PROJECT_SOURCE_DIR}/../../src
  ${CMAKE_CURRENT_BINARY_DIR}/../../include/)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../lib)

set (SRC_CPP_LIST
  IpConfiguratorCreator.cpp
  IpCatalogTree.cpp
  IpConfigWidget.cpp
  IpConfigurator.cpp
  IpInstancesTree.cpp
  IpTreesWidget.cpp
  IPDialogBox.cpp
)

set (SRC_H_INSTALL_LIST
  IpConfiguratorCreator.h
)

set (SRC_H_LIST
  ${SRC_H_INSTALL_LIST}
  IpCatalogTree.h
  IpConfigWidget.h
  IpConfigurator.h
  IpInstancesTree.h
  IpTreesWidget.h
  IPDialogBox.h
)

set (SRC_UI_LIST
  IPDialogBox.ui
)

add_library(ipconfigurator STATIC
  ${SRC_CPP_LIST}
  ${SRC_H_LIST}
  ${SRC_UI_LIST}
  ip_resources.qrc
)

target_link_libraries(ipconfigurator PUBLIC Qt5::Widgets Qt5::Core Qt5::Gui tcl_stubb tcl_static zlib)
target_compile_definitions(ipconfigurator PRIVATE IPCONFIGURATOR_LIBRARY)

install (
	FILES ${CMAKE_CURRENT_BINARY_DIR}/../../lib/$<TARGET_FILE_NAME:ipconfigurator>
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/foedag)

foreach(file ${SRC_H_INSTALL_LIST})
  install(
      FILES ${PROJECT_SOURCE_DIR}/../IpConfigurator/${file}
      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/foedag/IpConfigurator)
endforeach()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../bin)

add_executable(ipconfigurator_bin
 ${PROJECT_SOURCE_DIR}/../IpConfigurator/Test/ipconfigurator_main.cpp
 )
target_link_libraries(ipconfigurator_bin foedag tcl_stubb tcl_static zlib tclutils ipconfigurator)
set_target_properties(ipconfigurator_bin PROPERTIES OUTPUT_NAME ipconfigurator)

