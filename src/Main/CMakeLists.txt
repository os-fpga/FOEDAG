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
  ../Main/registerTclCommands.cpp
  ../Main/Settings.cpp
  ../Main/Tasks.cpp
  ../Main/WidgetFactory.cpp
  ../Main/FoedagStyle.cpp
  ../Main/ProjectFile/ProjectFileLoader.cpp
  ../Main/ProjectFile/ProjectFileComponent.cpp
  ../Main/ProjectFile/TaskManagerComponent.cpp
  ../Main/ProjectFile/ProjectManagerComponent.cpp
  ../Main/ProjectFile/CompilerComponent.cpp
  ../Main/AboutWidget.cpp
  ../Main/licenseviewer.cpp
  ../MainWindow/mainwindowmodel.cpp
  ../MainWindow/MessagesTabWidget.cpp
  ../MainWindow/ReportsTreeWidget.cpp
  ../MainWindow/WelcomePageWidget.cpp
  ../Main/TclSimpleParser.cpp
  CompilerNotifier.cpp
  ../MainWindow/PathEdit.cpp
  ../Main/DialogProvider.cpp
  ../MainWindow/MessageItemParser.cpp
  ../MainWindow/DockWidget.cpp
  ../MainWindow/LicenseManagerWidget.cpp
  ../Main/ReportGenerator.cpp
  ../Main/JsonReportGenerator.cpp
  ../MainWindow/EditorSettings.cpp
  ../MainWindow/Dialog.cpp
  ../MainWindow/CompressProject.cpp
  ../MainWindow/PerfomanceTracker.cpp
  ../Main/ProjectFile/FileLoaderOldStructure.cpp
  ../Main/ProjectFile/ProjectManagerComponentMigration.cpp
  ../MainWindow/QObjectContainer.cpp
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
  ../Main/ProjectFile/ProjectFileLoader.h
  ../Main/ProjectFile/ProjectFileComponent.h
  ../Main/ProjectFile/TaskManagerComponent.h
  ../Main/ProjectFile/ProjectManagerComponent.h
  ../Main/ProjectFile/CompilerComponent.h
  ../Main/AboutWidget.h
  ../Main/licenseviewer.h
  ../MainWindow/mainwindowmodel.h
  ../MainWindow/MessagesTabWidget.h
  ../MainWindow/ReportsTreeWidget.h
  ../MainWindow/TopLevelInterface.h
  ../MainWindow/WelcomePageWidget.h
  CompilerNotifier.h
  ../Main/TclSimpleParser.h
  ../MainWindow/PathEdit.h
  ../Main/DialogProvider.h
  ../MainWindow/MessageItemParser.h
  ../MainWindow/DockWidget.h
  ../MainWindow/LicenseManagerWidget.h
  ../Main/ReportGenerator.h
  ../Main/JsonReportGenerator.h
  ../MainWindow/EditorSettings.h
  ../MainWindow/Dialog.h
  ../MainWindow/CompressProject.h
  ../MainWindow/PerfomanceTracker.h
  ../Main/ProjectFile/FileLoaderOldStructure.h
  ../Main/ProjectFile/ProjectManagerComponentMigration.h
  ../MainWindow/QObjectContainer.h
)

set (SRC_UI_LIST ../MainWindow/WelcomePageWidget.ui)

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

target_link_libraries(foedagcore PUBLIC Qt6::Widgets Qt6::Core Qt6::Gui Qt6::Xml compiler)
if (USE_IPA)
  target_link_libraries(foedagcore PUBLIC interactive_path_analysis)
endif()

if(USE_MONACO_EDITOR)
  target_link_libraries(foedagcore PUBLIC Qt6::WebEngineWidgets)
  target_compile_definitions(foedagcore PUBLIC USE_MONACO_EDITOR=${USE_MONACO_EDITOR})
endif()

if (USE_IPA) 
  find_package(Qt6 COMPONENTS Network REQUIRED)
  if (NOT Qt6Network_FOUND)
      message(FATAL_ERROR "Failed to find Qt6Network")
  endif()
  target_link_libraries(foedagcore PUBLIC Qt6::Network)
endif()
###

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
          ${PROJECT_SOURCE_DIR}/../MainWindow/PerfomanceTracker.h
          ${PROJECT_SOURCE_DIR}/../MainWindow/QObjectContainer.h
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
    FILES ${PROJECT_SOURCE_DIR}/../Main/AboutWidget.h
    FILES ${PROJECT_SOURCE_DIR}/../Main/DialogProvider.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/foedag/Main/)
  
install(
    FILES ${PROJECT_SOURCE_DIR}/../Command/Command.h
    ${PROJECT_SOURCE_DIR}/../Command/Logger.h
    ${PROJECT_SOURCE_DIR}/../Command/CommandStack.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/foedag/Command)

