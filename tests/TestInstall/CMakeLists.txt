# -*- mode:cmake -*-

#Copyright 2021 The Foedag team

#GPL License

#Copyright (c) 2021 The Open-Source FPGA Foundation

#This program is free software: you can redistribute it and/or modify
#it under the terms of the GNU General Public License as published by
#the Free Software Foundation, either version 3 of the License, or
#(at your option) any later version.

#This program is distributed in the hope that it will be useful,
#but WITHOUT ANY WARRANTY; without even the implied warranty of
#MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#GNU General Public License for more details.

#You should have received a copy of the GNU General Public License
#along with this program.  If not, see <http://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.15)

# Usage:
# After a make install at the top drectory:
# cd tests/TestInstall;
# rm -rf build;
# mkdir -p build;
# cd build;
# cmake ../ -DINSTALL_DIR=/usr/local;
# make
# ./foedag --version

project(HELLOFOEDAG)

# NOTE: Policy changes has to happen before adding any subprojects
cmake_policy(SET CMP0091 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")


find_package(QT NAMES Qt5 COMPONENTS Core Quick REQUIRED)
find_package(Qt5 COMPONENTS Core Quick Gui Widgets Xml REQUIRED)
if (NOT Qt5Widgets_FOUND)
    message(STATUS "Failed to find Qt5Widgets required (on debian/ubuntu try 'sudo apt install qt5-default')")
elseif (NOT Qt5Gui_FOUND)
    message(STATUS "Failed to find Qt5Gui required (on debian/ubuntu try 'sudo apt install qt5-default')")
elseif (NOT Qt5Core_FOUND)
    message(STATUS "Failed to find Qt5Core required (on debian/ubuntu try 'sudo apt install qt5-default')")
elseif (NOT Qt5Xml_FOUND)
    message(STATUS "Failed to find Qt5Xml required (on debian/ubuntu try 'sudo apt install qt5-default')")
else()
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTORCC ON)
endif()


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Python
if (FOEDAG_WITH_PYTHON)
find_package(Python3 3.3 REQUIRED COMPONENTS Interpreter Development)
message(STATUS "Python3_LIBRARIES = ${Python3_LIBRARIES}")
message(STATUS "Python3_EXECUTABLE = ${Python3_EXECUTABLE}")
message(STATUS "Python3_INCLUDE_DIRS = ${Python3_INCLUDE_DIRS}")
message(STATUS "Python3_RUNTIME_LIBRARY_DIRS = ${Python3_RUNTIME_LIBRARY_DIRS}")
endif()

if (MSVC)
  set(TCL_STUBB_LIB tclstub86.lib)
  set(TCL_STATIC_LIB tcl86ts.lib)
  set(ZLIB_STATIC_LIB zlib.lib)
  add_library(tcl_static STATIC IMPORTED)
else()
  set(TCL_STATIC_LIB libtcl8.6.so)
  set(TCL_STUBB_LIB libtclstub8.6.a)
  set(ZLIB_STATIC_LIB libz.a)

  if(APPLE) 
    set(TCL_STATIC_LIB libtcl8.6.dylib)
  endif()
  
  add_library(tcl_static SHARED IMPORTED)

  if(CMAKE_SYSTEM_NAME MATCHES "MSYS")
    set(TCL_STATIC_LIB libtcl8.6.a)
    add_library(tcl_static STATIC IMPORTED)
  endif()
  
endif()

ADD_LIBRARY(tcl_stubb STATIC IMPORTED)

SET_TARGET_PROPERTIES(tcl_static  PROPERTIES
  IMPORTED_LOCATION ${INSTALL_DIR}/lib/foedag/lib/${TCL_STATIC_LIB})
  
SET_TARGET_PROPERTIES(tcl_stubb  PROPERTIES
  IMPORTED_LOCATION ${INSTALL_DIR}/lib/foedag/lib/${TCL_STUBB_LIB})   


ADD_LIBRARY(zlib STATIC IMPORTED)
SET_TARGET_PROPERTIES(zlib  PROPERTIES
    IMPORTED_LOCATION ${INSTALL_DIR}/lib/foedag/lib/${ZLIB_STATIC_LIB})

if(NOT NO_TCMALLOC)
  find_library(TCMALLOC_LIBRARY NAMES tcmalloc)
endif()

# Directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${TCMALLOC_COMPILE_OPTIONS} ${MY_CXX_WARNING_FLAGS}")
if(MSVC)
  add_compile_definitions(_CRT_NONSTDC_NO_WARNINGS)
  set(CMAKE_CXX_FLAGS_DEBUG  "${CMAKE_CXX_FLAGS_DEBUG} ${TCMALLOC_COMPILE_OPTIONS} /Zc:__cplusplus /W0 /bigobj /GL- /DSTATIC_BUILD ${MY_CXX_WARNING_FLAGS}")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${TCMALLOC_COMPILE_OPTIONS} /Zc:__cplusplus /W0 /bigobj /GL- /DSTATIC_BUILD ${MY_CXX_WARNING_FLAGS}")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /STACK:8388608")  # 8MB stack size
else()
  # C++ Compilation line
  if($<BOOL:${TCMALLOC_LIBRARY}>)
    set (TCMALLOC_COMPILE_OPTIONS "-fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free")
  endif($<BOOL:${TCMALLOC_LIBRARY}>)

  set(CMAKE_CXX_FLAGS_DEBUG  "${CMAKE_CXX_FLAGS_DEBUG} ${TCMALLOC_COMPILE_OPTIONS} -Wall -O0 -g ${MY_CXX_WARNING_FLAGS}")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${TCMALLOC_COMPILE_OPTIONS} -Wall -O3 -DNDEBUG ${MY_CXX_WARNING_FLAGS}")
endif()

add_executable(test_hellofoedag ${PROJECT_SOURCE_DIR}/../../src/Main/main.cpp)

if(MSVC)
  set_property(TARGET test_hellofoedag PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
  set_property(TARGET test_hellofoedag PROPERTY COMPILER_FLAGS /DSTATIC_BUILD)
endif()

target_include_directories(test_hellofoedag PRIVATE
  ${INSTALL_DIR}/include/foedag
  ${INSTALL_DIR}/include
  ${INSTALL_DIR}/include/foedag/include
  ${Python3_INCLUDE_DIRS}
)
target_link_directories(test_hellofoedag
    PRIVATE ${INSTALL_DIR}/lib/foedag
)

target_link_libraries(test_hellofoedag
  foedagcore
  foedag
  newfile
  newproject
  projnavigator
  designruns
  texteditor
  console
  compiler
  ipgenerate
  QConsole
  qscintilla2_qt
  tcl_stubb
  tcl_static
  zlib
  $<$<BOOL:${FOEDAG_WITH_PYTHON}>:${Python3_LIBRARIES}>
  Qt5::Widgets Qt5::Core Qt5::Gui Qt5::Xml Qt5::Quick
  $<$<BOOL:${TCMALLOC_LIBRARY}>:tcmalloc>
)

if(MSVC)
  SET_TARGET_PROPERTIES(tcl_static PROPERTIES
    IMPORTED_LOCATION ${INSTALL_DIR}/lib/foedag/lib/${TCL_STATIC_LIB})
  set_target_properties(tcl_static PROPERTIES
    COMPILE_OPTIONS "/MT"
  )
  SET_TARGET_PROPERTIES(tcl_stubb PROPERTIES
    IMPORTED_LOCATION ${INSTALL_DIR}/lib/foedag/lib/${TCL_STUBB_LIB})
  set_target_properties(tcl_stubb PROPERTIES
    COMPILE_OPTIONS "/MT>"
  )
  set_target_properties(test_hellofoedag PROPERTIES
    COMPILE_OPTIONS "/MT"
  )
  target_link_libraries(test_hellofoedag Netapi32)
endif()

if (APPLE)
  target_link_libraries(test_hellofoedag
      dl
      util
      m
      pthread
      "-framework CoreFoundation"
  )
elseif (UNIX)
  target_link_libraries(test_hellofoedag
      stdc++fs
      dl
      util
      m
      rt
      pthread
  )
endif()

if (WIN32)
  if (FOEDAG_WITH_PYTHON)
    add_custom_command(
      TARGET test_hellofoedag
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${Python3_RUNTIME_LIBRARY_DIRS}/python${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR}$<$<CONFIG:Debug>:_d>.dll
            $<TARGET_FILE_DIR:test_hellosureworld>)
    endif()
endif()

add_custom_target(RunInstallTest ALL
    DEPENDS test_hellofoedag
    COMMAND $<TARGET_FILE:test_hellofoedag> --batch --cmd \"puts Hello!\"
    WORKING_DIRECTORY $<TARGET_FILE_DIR:test_hellofoedag>)
