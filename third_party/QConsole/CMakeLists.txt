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

project(QConsole LANGUAGES CXX)

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

include_directories(${PROJECT_SOURCE_DIR}/../../src ${PROJECT_SOURCE_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/../../include/)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../lib)

set (SRC_CPP_LIST
  src/qconsole.cpp
)

set (SRC_H_LIST
  include/qconsole.h
)

add_library(QConsole STATIC
  ${SRC_CPP_LIST}
  ${SRC_H_LIST}
)

target_link_libraries(QConsole PUBLIC Qt5::Widgets Qt5::Core Qt5::Gui)
add_custom_command(
  TARGET QConsole POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy
          ${PROJECT_SOURCE_DIR}/include/qconsole.h
          ${CMAKE_CURRENT_BINARY_DIR}/../../include/QConsole/qconsole.h
)

install (
  FILES ${CMAKE_CURRENT_BINARY_DIR}/../../lib/$<TARGET_FILE_NAME:QConsole>
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/foedag)

install(
   FILES ${PROJECT_SOURCE_DIR}/include/qconsole.h
   DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/foedag/QConsole)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../bin)
