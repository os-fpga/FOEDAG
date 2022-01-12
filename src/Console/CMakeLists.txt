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
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_RELEASE} -Werror")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Werror")
endif()

include (../../cmake/cmake_qt.txt)
include (../../cmake/cmake_tcl.txt)

include_directories(${PROJECT_SOURCE_DIR}/../../src ${PROJECT_SOURCE_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/../../include/)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../lib)

set (SRC_CPP_LIST
  ConsoleWidget.cpp
  ConsoleInterface.cpp
  TclController.cpp
  TclConsole.cpp
  TclWorker.cpp
  StreamBuffer.cpp
)

set (SRC_H_LIST
  ConsoleWidget.h
  ConsoleInterface.h
  TclController.h
  TclConsole.h
  TclWorker.h
  StreamBuffer.h
)

set (SRC_UI_LIST
)

add_library(console STATIC
  ${SRC_CPP_LIST}
  ${SRC_H_LIST}
  ${SRC_UI_LIST}
)

target_link_libraries(console PUBLIC Qt5::Widgets Qt5::Core Qt5::Gui Qt5::Xml)
target_compile_definitions(console PRIVATE CONSOLE_LIBRARY)

set(CONSOLE_STATIC_LIB libconsole.a)
if (MSVC)
  set(CONSOLE_STATIC_LIB console.lib)
endif()

install (
  FILES ${CMAKE_CURRENT_BINARY_DIR}/../../lib/${CONSOLE_STATIC_LIB}
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/foedag)

install(
   FILES ${PROJECT_SOURCE_DIR}/../Console/ConsoleWidget.h
         ${PROJECT_SOURCE_DIR}/../Console/TclController.h
         ${PROJECT_SOURCE_DIR}/../Console/ConsoleInterface.h
         ${PROJECT_SOURCE_DIR}/../Console/TclConsole.h
   DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/foedag/Console)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../bin)

add_executable(console_bin
  ${PROJECT_SOURCE_DIR}/../Console/Test/console_main.cpp)
target_link_libraries(console_bin foedag compiler tcl_stubb tcl_static zlib console)
set_target_properties(console_bin PROPERTIES OUTPUT_NAME console)
