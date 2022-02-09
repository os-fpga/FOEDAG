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

project(foedagcore LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt5 COMPONENTS Core Quick REQUIRED)
find_package(Qt5 COMPONENTS Core Gui Widgets Xml Quick REQUIRED)
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

if(MSVC)
  set(TCL_STUBB_LIB tclstub90.lib)
  set(TCL_STATIC_LIB tcl90s.lib)
  set(ZLIB_STATIC_LIB zlib.lib)
  ADD_LIBRARY(tcl_static STATIC IMPORTED)
  SET_TARGET_PROPERTIES(tcl_static PROPERTIES
    IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/../../lib/${TCL_STATIC_LIB})
  set_target_properties(tcl_static PROPERTIES
    COMPILE_OPTIONS "$<$<CONFIG:Debug>:/MTd>$<$<CONFIG:Release>:/MT>"
  )  
  ADD_LIBRARY(tcl_stubb STATIC IMPORTED)
  SET_TARGET_PROPERTIES(tcl_stubb PROPERTIES
    IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/../../lib/${TCL_STUBB_LIB})
  set_target_properties(tcl_stubb PROPERTIES
    COMPILE_OPTIONS "$<$<CONFIG:Debug>:/MTd>$<$<CONFIG:Release>:/MT>"
  )  

  set(CMAKE_CXX_FLAGS_DEBUG
      "${CMAKE_CXX_FLAGS_DEBUG} ${TCMALLOC_COMPILE_OPTIONS} /Zc:__cplusplus /W0 /bigobj /Zl /GL- /MT /DSTATIC_BUILD ${MY_CXX_WARNING_FLAGS}"
  )
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO
      "${CMAKE_CXX_FLAGS_RELEASE} ${TCMALLOC_COMPILE_OPTIONS} /Zc:__cplusplus /W0 /bigobj /Zl /GL- /MT /DSTATIC_BUILD ${MY_CXX_WARNING_FLAGS}"
  )
  set(CMAKE_CXX_FLAGS_RELEASE
      "${CMAKE_CXX_FLAGS_RELEASE} ${TCMALLOC_COMPILE_OPTIONS} /Zc:__cplusplus /W0 /bigobj /Zl /GL- /MT /DSTATIC_BUILD ${MY_CXX_WARNING_FLAGS}"
  )
  set(CMAKE_EXE_LINKER_FLAGS /STACK:8388608)  # 8MB stack size
else()
  set(TCL_STATIC_LIB libtcl9.0.a)
  set(TCL_STUBB_LIB libtclstub9.0.a)
  set(ZLIB_STATIC_LIB libz.a)
  ADD_LIBRARY(tcl_static STATIC IMPORTED)
  SET_TARGET_PROPERTIES(tcl_static PROPERTIES
    IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/../../lib/${TCL_STATIC_LIB})
  ADD_LIBRARY(tcl_stubb STATIC IMPORTED)
  SET_TARGET_PROPERTIES(tcl_stubb PROPERTIES
    IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/../../lib/${TCL_STUBB_LIB})
  ADD_LIBRARY(zlib STATIC IMPORTED)
  SET_TARGET_PROPERTIES(zlib PROPERTIES
    IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/../../lib/${ZLIB_STATIC_LIB})       
endif()

include_directories(
  ${PROJECT_SOURCE_DIR}/../../src
  ${CMAKE_CURRENT_BINARY_DIR}/../../include/)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../lib)

set (SRC_CPP_LIST ../Main/Foedag.cpp
  ../Tcl/TclInterpreter.cpp
  ../Tcl/TclHistoryScript.cpp
  ../Command/Command.cpp
  ../Command/CommandStack.cpp
  ../Command/Logger.cpp
  ../MainWindow/main_window.cpp
  ../MainWindow/Session.cpp
  ../Main/qttclnotifier.cpp
  ../Main/CommandLine.cpp
  ../Main/registerTclCommands.cpp)

set (SRC_H_LIST ../Main/Foedag.h
  ../Tcl/TclInterpreter.h
  ../Command/Command.h 
  ../Command/CommandStack.h
  ../Command/Logger.h
  ../MainWindow/main_window.h
  ../MainWindow/Session.h
  ../Main/qttclnotifier.hpp
  ../Main/CommandLine.h)

set (SRC_UI_LIST "")

set (SRC_QRC_LIST ../MainWindow/main_window_resource.qrc
    ../MainWindow/qml.qrc)

add_library(foedagcore STATIC
    ${SRC_CPP_LIST}
    ${SRC_H_LIST}
    ${SRC_UI_LIST}
    ${SRC_QRC_LIST}
)


if(MSVC)
  set_property(TARGET foedagcore PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  set_property(TARGET foedagcore PROPERTY COMPILER_FLAGS /DSTATIC_BUILD)
  set_target_properties(foedagcore PROPERTIES
    COMPILE_OPTIONS "$<$<CONFIG:Debug>:/MTd>$<$<CONFIG:Release>:/MT>"
  )
endif()

if (MSVC)
else()
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_RELEASE} -Wno-deprecated-declarations -Werror")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wno-deprecated-declarations -Werror")
endif()

target_link_libraries(foedagcore PUBLIC tcl_stubb tcl_static zlib Qt5::Widgets Qt5::Core Qt5::Gui Qt5::Xml Qt5::Quick)
target_compile_definitions(foedagcore PRIVATE FOEDAG_CORE_LIBRARY)

set(FOEDAG_CORE_STATIC_LIB libfoedagcore.a)
if (MSVC)
  set(FOEDAG_CORE_STATIC_LIB foedagcore.lib)
endif()

install (
  FILES ${CMAKE_CURRENT_BINARY_DIR}/../../lib/${FOEDAG_CORE_STATIC_LIB}
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/foedag)

install(
    FILES ${PROJECT_SOURCE_DIR}/../MainWindow/main_window.h
          ${PROJECT_SOURCE_DIR}/../MainWindow/Session.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/foedag/MainWindow)
  
install(
    FILES ${PROJECT_SOURCE_DIR}/../Tcl/TclInterpreter.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/foedag/Tcl)
  
install(
    FILES ${PROJECT_SOURCE_DIR}/../Main/CommandLine.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/foedag/Main/)
  
install(
    FILES ${PROJECT_SOURCE_DIR}/../Command/Command.h
    ${PROJECT_SOURCE_DIR}/../Command/Logger.h
    ${PROJECT_SOURCE_DIR}/../Command/CommandStack.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/foedag/Command)

