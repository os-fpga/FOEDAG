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
  ProgrammerMain.cpp
)

set (SRC_H_INSTALL_LIST
  ProgrammerMain.h
)

set (SRC_UI
  ProgrammerMain.ui
)

set (SRC_H_LIST
  ${SRC_H_INSTALL_LIST}
)

set (res_LIST
#  res.qrc
)

add_library(programmer-gui STATIC
  ${SRC_CPP_LIST}
  ${SRC_H_LIST}
  ${res_LIST}
  ${SRC_UI}
)

target_link_libraries(programmer-gui PUBLIC Qt5::Widgets Qt5::Core Qt5::Gui Qt5::Xml)
target_compile_definitions(programmer-gui PRIVATE PROGRAMMER_GUI_LIBRARY)

install (
  FILES ${CMAKE_CURRENT_BINARY_DIR}/../../lib/$<TARGET_FILE_NAME:programmer-gui>
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/foedag)

foreach(file ${SRC_H_INSTALL_LIST})
  install(
      FILES ${PROJECT_SOURCE_DIR}/../ProgrammerGui/${file}
      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/foedag/ProgrammerGui)
endforeach()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../bin)

add_executable(programmer-gui_test
  ${PROJECT_SOURCE_DIR}/../ProgrammerGui/Test/programmer_gui_main.cpp
  ${PROJECT_SOURCE_DIR}/../ProgrammerGui/Test/ProgrammerGuiCommands.cpp)
target_link_libraries(programmer-gui_test foedag foedagcore tcl_stubb tcl_static
  zlib tclutils programmer-gui)
set_target_properties(programmer-gui_test PROPERTIES OUTPUT_NAME programmer-gui_test)

