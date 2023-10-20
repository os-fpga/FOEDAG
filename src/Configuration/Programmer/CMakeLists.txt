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
set(subsystem programmer)
set(foedag_bin foedag_${subsystem})

project(${subsystem} LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
else()
  set(CMAKE_CXX_FLAGS_DEBUG
  "${CMAKE_CXX_FLAGS_DEBUG} ${TCMALLOC_COMPILE_OPTIONS} -Werror -Wall -O0 -g ${MSYS_COMPILE_OPTIONS} ${MY_CXX_WARNING_FLAGS} ${MEM_SANITIZER_FLAGS}")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Werror")
endif()

#################################################################################################################
#
# Three elements will be built 
#
# 1. subsystem library (in this case programmer) - it contains all the core code to generate bitstream. It will be called by other elements
#
# 2. foedag_bin executable (in this case foedag_programmer exe) - it is external command line entry which eventually call subsystem library (item 1)
#
#################################################################################################################

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../../../cmake/")
find_package(USB1 REQUIRED)
include_directories(${LIBUSB_INCLUDE_DIR})
###################
#
# subsystem library
#
###################
add_library(
  ${subsystem} ${CFG_LIB_TYPE}
  Programmer.cpp
  Programmer_helper.cpp
  HwDevices.cpp
  ProgrammerGuiInterface.h
)
target_link_libraries(${subsystem} PRIVATE ${LIBUSB_LIBRARIES} foedagutils)
###################
#
# foedag_bin which also has dependency on its own subsystem library
#
###################
add_executable(
  ${foedag_bin}
  Programmer_cmd.cpp
  Programmer_helper.cpp
  HwDevices.cpp
  ProgrammerGuiInterface.h
)
target_link_libraries(${foedag_bin} ${subsystem})

###################
#
# install 
#   - for header file(s), only put those that we really need to export
#
###################
set(
  SRC_H_INSTALL_LIST
)

set(
  SRC_LIB_INSTALL_LIST
  ${subsystem}
)

foreach(file ${SRC_LIB_INSTALL_LIST})
  install(
    FILES ${CFG_BUILD_ROOT_DIR}/lib/$<TARGET_FILE_NAME:${file}>
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/foedag
  )
endforeach()

foreach(file ${SRC_H_INSTALL_LIST})
  install(
    FILES ${PROJECT_SOURCE_DIR}/${file}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/foedag/Programmer
  )
endforeach()


# Message(STATUS "## hello 1 ## CMAKE_RUNTIME_OUTPUT_DIRECTORY" : "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
# Message(STATUS "## hello 2 ## CMAKE_INSTALL_BINDIR" : "${CMAKE_INSTALL_BINDIR}")
# Message(STATUS "## hello 3 ## CMAKE_INSTALL_INCLUDEDIR" : "${CMAKE_INSTALL_INCLUDEDIR}")

# if(1)
#   set(VCPKG_LIBUSB_DIR "$ENV{VCPKG_INSTALLATION_ROOT}/packages/libusb_x64-windows")
#   file(TO_CMAKE_PATH "${VCPKG_LIBUSB_DIR}" VCPKG_LIBUSB_DIR)
#   set(libusb_dll "${VCPKG_LIBUSB_DIR}/bin/libusb-1.0.dll")
#   add_custom_target(copy_libusb_dll_to_bin
#     COMMAND ${CMAKE_COMMAND} -E copy_if_different ${libusb_dll} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
#     DEPENDS ${libusb_dll}
#   )
#   add_dependencies(${foedag_bin} copy_libusb_dll_to_bin)

#   install(
#     FILES ${libusb_dll} 
#     DESTINATION ${CMAKE_INSTALL_BINDIR})
# endif()
