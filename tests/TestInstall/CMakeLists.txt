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

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/../../cmake/")
find_package(USB1 REQUIRED)
#find_package(QT NAMES Qt6 COMPONENTS Core Quick REQUIRED)
#find_package(Qt6 COMPONENTS Core Quick Gui Widgets Xml REQUIRED)

if (MONACO_EDITOR)
  message("Using Monaco Editor")
  set(USE_MONACO_EDITOR ON CACHE BOOL "" FORCE)
else(MONACO_EDITOR)
  message("Using QScintilla Editor")
  set(USE_MONACO_EDITOR OFF CACHE BOOL "" FORCE)
endif(MONACO_EDITOR)

if(USE_MONACO_EDITOR)
    find_package(Qt6 COMPONENTS Core Gui Widgets Xml Network WebEngineWidgets WebChannel REQUIRED)
else()
    find_package(Qt6 COMPONENTS Core Gui Widgets Xml Network REQUIRED)
endif()
if (NOT Qt6Widgets_FOUND)
    message(STATUS "Failed to find Qt6Widgets required (on debian/ubuntu try 'sudo apt install qt6-base-dev')")
elseif (NOT Qt6Gui_FOUND)
    message(STATUS "Failed to find Qt6Gui required (on debian/ubuntu try 'sudo apt install qt6-base-dev')")
elseif (NOT Qt6Core_FOUND)
    message(STATUS "Failed to find Qt6Core required (on debian/ubuntu try 'sudo apt install qt6-base-dev')")
elseif (NOT Qt6Xml_FOUND)
    message(STATUS "Failed to find Qt6Xml required (on debian/ubuntu try 'sudo apt install qt6-base-dev')")
elseif (NOT Qt6Network_FOUND)
    message(STATUS "Failed to find Qt6Network required (on debian/ubuntu try 'sudo apt install qt6-base-dev')")
elseif (USE_MONACO_EDITOR AND NOT Qt6WebEngineWidgets_FOUND)
    message(STATUS "Failed to find Qt6WebEngineWidgets required (on debian/ubuntu try 'sudo apt install qt6-webengine-dev qt6-webengine* libqt6webenginecore6*')")
elseif (USE_MONACO_EDITOR AND NOT Qt6WebChannel_FOUND)
    message(STATUS "Failed to find Qt6WebChannel required (on debian/ubuntu try 'sudo apt install qt6-webengine-dev qt6-webengine* libqt6webenginecore6*')")
else()
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTORCC ON)
endif()

if (IPA)
  message("Enable Interactive Path Analysis Tool")
  set(USE_IPA ON CACHE BOOL "" FORCE)
  add_definitions(-DUSE_IPA)
else(IPA)
  message("Disable Interactive Path Analysis Tool")
  set(USE_IPA OFF CACHE BOOL "" FORCE)
endif(IPA)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(FOEDAG_WITH_PYTHON ON)

# Python
if (FOEDAG_WITH_PYTHON)
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/")
  find_package(CustomPython3 REQUIRED)
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
    add_library(tcl_static SHARED IMPORTED )
  elseif ((DEFINED ENV{MSYSTEM}) AND ("$ENV{MSYSTEM}" STREQUAL "MINGW64"))
    set(TCL_STATIC_LIB libtcl86.dll.a)
    set(ZLIB_STATIC_LIB libzlibstatic.a)
    set(TCL_STUBB_LIB libtclstub86.a)
    add_library(tcl_static STATIC IMPORTED)
  else()
    add_library(tcl_static SHARED IMPORTED )
  endif()

endif()

ADD_LIBRARY(tcl_stubb STATIC IMPORTED)

SET_TARGET_PROPERTIES(tcl_static PROPERTIES
  IMPORTED_LOCATION ${INSTALL_DIR}/lib/foedag/lib/${TCL_STATIC_LIB})

SET_TARGET_PROPERTIES(tcl_stubb PROPERTIES
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
  simulation
  ipgenerate
  designquery
  devicemodeling
  QConsole
  tcl_stubb
  tcl_static
  zlib
  ipconfigurator
  foedagutils
  pinassignment
  cfgcompiler
  modelconfig
  cfgcommon
  programmer
  hardwaremanager
  rapidgpt
  ${LIBUSB_LIBRARIES}
  $<$<BOOL:${FOEDAG_WITH_PYTHON}>:${Python3_LIBRARIES}>
#  Qt6::Quick
  $<$<BOOL:${TCMALLOC_LIBRARY}>:tcmalloc>
)

if (USE_MONACO_EDITOR)
  target_link_libraries(test_hellofoedag Qt6::WebEngineWidgets Qt6::WebChannel)
else()
  target_link_libraries(test_hellofoedag qscintilla2_qt)
endif()

if (USE_IPA)
  target_link_libraries(test_hellofoedag interactive_path_analysis zlib)
endif()

target_link_libraries(test_hellofoedag Qt6::Widgets Qt6::Core Qt6::Gui Qt6::Xml Qt6::Network)

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

if (MSVC)
  if (FOEDAG_WITH_PYTHON)
    add_custom_command(
      TARGET test_hellofoedag
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${Python3_RUNTIME_LIBRARY_DIRS}/python${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR}$<$<CONFIG:Debug>:_d>.dll
            $<TARGET_FILE_DIR:test_hellofoedag>)
    endif()
endif()

add_custom_target(RunInstallTest ALL
    DEPENDS test_hellofoedag
    COMMAND $<TARGET_FILE:test_hellofoedag> --batch --cmd \"puts Hello!\"
    WORKING_DIRECTORY $<TARGET_FILE_DIR:test_hellofoedag>)

if(MSVC)
  set(VCPKG_LIBUSB_DIR "$ENV{VCPKG_INSTALLATION_ROOT}/packages/libusb_x64-windows")
  file(TO_CMAKE_PATH "${VCPKG_LIBUSB_DIR}" VCPKG_LIBUSB_DIR)
  set(libusb_dll "${VCPKG_LIBUSB_DIR}/bin/libusb-1.0.dll")
  file(COPY ${libusb_dll} DESTINATION  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
endif()
