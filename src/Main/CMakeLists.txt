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

include (../../cmake/cmake_qt.txt)
include (../../cmake/cmake_tcl.txt)

include_directories(
  ${PROJECT_SOURCE_DIR}/../../src
  ${CMAKE_CURRENT_BINARY_DIR}/../../include/)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../lib)

#set (SRC_CPP_LIST ../Main/Foedag.cpp
set (SRC_CPP_LIST ../Main/Foedag_ql.cpp
  ../Tcl/TclInterpreter.cpp
  ../Tcl/TclHistoryScript.cpp
  ../Command/Command.cpp
  ../Command/CommandStack.cpp
  ../Command/Logger.cpp
  #../MainWindow/main_window.cpp
  ../MainWindow/main_window_ql.cpp
  ../MainWindow/Session.cpp
  ../Main/qttclnotifier.cpp
  ../Main/CommandLine.cpp
  ../Main/registerTclCommands.cpp
  ../Main/Settings.cpp
  ../Main/Tasks.cpp
  ../Main/WidgetFactory.cpp
  ../Main/FoedagStyle.cpp
  ../MainWindow/mainwindowmodel.cpp
  CompilerNotifier.cpp
)

set (SRC_H_LIST ../Main/Foedag.h
  ../Tcl/TclInterpreter.h
  ../Command/Command.h 
  ../Command/CommandStack.h
  ../Command/Logger.h
  ../MainWindow/main_window.h
  ../MainWindow/Session.h
  ../Main/qttclnotifier.hpp
  ../Main/CommandLine.h
  ../Main/Settings.h
  ../Main/Tasks.h
  ../Main/WidgetFactory.h
  ../Main/FoedagStyle.h
  ../MainWindow/mainwindowmodel.h
  ../MainWindow/TopLevelInterface.h
  CompilerNotifier.h
)

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
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-deprecated-declarations -Werror")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wno-deprecated-declarations -Werror")
endif()

target_link_libraries(foedagcore PUBLIC  Qt5::Widgets Qt5::Core Qt5::Gui Qt5::Xml Qt5::Quick compiler)
target_compile_definitions(foedagcore PUBLIC FOEDAG_CORE_LIBRARY)

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
          ${PROJECT_SOURCE_DIR}/../MainWindow/mainwindowmodel.h
          ${PROJECT_SOURCE_DIR}/../MainWindow/TopLevelInterface.h
          ${PROJECT_SOURCE_DIR}/../NewProject/newprojectmodel.h
          ${PROJECT_SOURCE_DIR}/../NewFile/newfilemodel.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/foedag/MainWindow)
  
install(
    FILES ${PROJECT_SOURCE_DIR}/../Tcl/TclInterpreter.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/foedag/Tcl)
  
install(
    FILES ${PROJECT_SOURCE_DIR}/../Main/CommandLine.h
    FILES ${PROJECT_SOURCE_DIR}/../Main/Foedag.h
    FILES ${PROJECT_SOURCE_DIR}/../Main/ToolContext.h
    FILES ${PROJECT_SOURCE_DIR}/../Main/Settings.h
    FILES ${PROJECT_SOURCE_DIR}/../Main/Tasks.h
    FILES ${PROJECT_SOURCE_DIR}/../Main/WidgetFactory.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/foedag/Main/)
  
install(
    FILES ${PROJECT_SOURCE_DIR}/../Command/Command.h
    ${PROJECT_SOURCE_DIR}/../Command/Logger.h
    ${PROJECT_SOURCE_DIR}/../Command/CommandStack.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/foedag/Command)

