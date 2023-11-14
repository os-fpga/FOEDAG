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
set(subsystem devicemodeling)

project(${subsystem} LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
else()
  set(CMAKE_CXX_FLAGS_DEBUG
    "${CMAKE_CXX_FLAGS_DEBUG} ${TCMALLOC_COMPILE_OPTIONS} -Werror -Wall -O0 -g ${MSYS_COMPILE_OPTIONS} ${MY_CXX_WARNING_FLAGS} ${MEM_SANITIZER_FLAGS}")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Werror")
endif()

include(../../cmake/cmake_qt.txt)
include(../../cmake/cmake_tcl.txt)

include_directories(${PROJECT_SOURCE_DIR}/../../src ${PROJECT_SOURCE_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/../../include/)

# Add the Google Test directory to the include path
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/googletest/googletest/include ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/googletest/googlemock/include)

# Add the Google Test library
# add_subdirectory(${PROJECT_SOURCE_DIR}/../../../third_party/googletest ${CMAKE_CURRENT_BINARY_DIR}/googletest)

# Add the spdlog directory to the include path
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/spdlog/include)

# Add the exprtk directory to the include path
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/exprtk)

# Add the source directory to the include path
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/device_spec ${CMAKE_CURRENT_SOURCE_DIR}/src/configuration ${CMAKE_CURRENT_SOURCE_DIR}/src/modeler)


set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../lib)

file (GLOB SRC_CPP_LIST PRIVATE
  "*.cpp"
)

set(SRC_H_INSTALL_LIST
  DeviceModeling.h
)

set(SRC_H_LIST
  ${SRC_H_INSTALL_LIST}
)

set(res_LIST
)

add_library(${subsystem} STATIC
  ${SRC_CPP_LIST}
  ${SRC_H_LIST}
  ${res_LIST}
)

target_compile_definitions(${subsystem} PUBLIC NOMINMAX=1)

target_link_libraries(${subsystem} PUBLIC Qt6::Widgets Qt6::Core Qt6::Gui Qt6::Xml)
target_compile_definitions(${subsystem} PRIVATE DESIGNQUERY_LIBRARY)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/../../lib/$<TARGET_FILE_NAME:${subsystem}>
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/foedag)

foreach(file ${SRC_H_INSTALL_LIST})
  install(
    FILES ${PROJECT_SOURCE_DIR}/../DeviceModeling/${file}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/foedag/DeviceModeling)
endforeach()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../bin)

add_executable(${subsystem}_bin
  ${PROJECT_SOURCE_DIR}/../DeviceModeling/Test/${subsystem}_main.cpp)
target_link_libraries(${subsystem}_bin foedag foedagcore tcl_stubb tcl_static zlib compiler simulation)
set_target_properties(${subsystem}_bin PROPERTIES OUTPUT_NAME ${subsystem}_test)
