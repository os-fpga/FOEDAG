# @file CMakeLists.txt
# @author Oleksandr Pyvovarov (APivovarov@quicklogic.com or aleksandr.pivovarov.84@gmail.com or
# https://github.com/w0lek)
# @date 2024-03-12
# @copyright Copyright 2021 The Foedag team

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

project(ipa LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
else()
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Werror")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Werror")
endif()

include (${CMAKE_SOURCE_DIR}/cmake/cmake_qt.txt)

add_definitions(-DTODO_IPA_MIGRATION_COMPILER)
add_definitions(-DTODO_IPA_MIGRATION_SETTINGS)

include_directories(${CMAKE_SOURCE_DIR}/src ${CMAKE_BINARY_DIR}/include)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set (SRC_CPP_LIST
  NCriticalPathReportParser.cpp
  NCriticalPathToolsWidget.cpp
  NCriticalPathParameters.cpp
  NCriticalPathItem.cpp
  NCriticalPathModel.cpp
  NCriticalPathFilterModel.cpp
  NCriticalPathView.cpp
  NCriticalPathWidget.cpp
  NCriticalPathItemDelegate.cpp
  NCriticalPathStatusBar.cpp
  NCriticalPathFilterWidget.cpp
  FilterCriteriaConf.cpp
  CustomMenu.cpp
  VprProcess.cpp
  client/ConvertUtils.cpp
  client/GateIO.cpp
  client/TcpSocket.cpp
  client/TelegramBuffer.cpp
  client/RequestCreator.cpp
  client/TelegramParser.cpp
  client/ZlibUtils.cpp
)

set (SRC_H_LIST
  NCriticalPathReportParser.h
  NCriticalPathToolsWidget.h
  NCriticalPathParameters.h
  NCriticalPathItem.h
  NCriticalPathModel.h
  NCriticalPathFilterModel.h
  NCriticalPathView.h
  NCriticalPathWidget.h
  NCriticalPathItemDelegate.h
  NCriticalPathStatusBar.h
  NCriticalPathFilterWidget.h
  NCriticalPathTheme.h
  NCriticalPathModuleInfo.h
  FilterCriteriaConf.h
  CustomMenu.h
  SimpleLogger.h
  VprProcess.h
  client/ConvertUtils.h
  client/GateIO.h
  client/ServerFreePortDetector.h
  client/CommConstants.h
  client/TcpSocket.h
  client/TelegramBuffer.h
  client/RequestCreator.h
  client/TelegramParser.h
  client/ZlibUtils.h
)

set (SRC_UI_LIST

)

add_library(interactive_path_analysis STATIC
  ${SRC_CPP_LIST}
  ${SRC_H_LIST}
  ${SRC_UI_LIST}
)

find_package(Qt6 COMPONENTS Network REQUIRED)
if (NOT Qt6Network_FOUND)
    message(FATAL_ERROR "Failed to find Qt6Network")
endif()

target_link_libraries(interactive_path_analysis PUBLIC Qt6::Widgets Qt6::Core Qt6::Gui Qt6::Network compiler)
if(MSVC)
  include_directories(${CMAKE_SOURCE_DIR}/third_party/zlib)
  include_directories(${CMAKE_BINARY_DIR}/third_party/zlib)
  # set(ZLIB_STATIC_LIB zlib.lib)
  # add_library(zlib STATIC IMPORTED)
  # set_target_properties(zlib PROPERTIES
  #   IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/lib/${ZLIB_STATIC_LIB})

  # target_link_libraries(interactive_path_analysis PUBLIC zlib)
else()
  find_package(ZLIB REQUIRED)
  target_link_libraries(interactive_path_analysis PUBLIC ZLIB::ZLIB)
endif()

target_compile_definitions(interactive_path_analysis PRIVATE INTERACTIVEPATHANALYSIS_LIBRARY)

install (
  FILES ${CMAKE_BINARY_DIR}/lib/$<TARGET_FILE_NAME:interactive_path_analysis>
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/foedag)

set (HEADER_INSTALL_LIST "")
foreach(HEADER ${SRC_H_LIST})
  list(APPEND HEADER_INSTALL_LIST ${PROJECT_SOURCE_DIR}/../InteractivePathAnalysis/${HEADER})
endforeach()

install(
 FILES ${HEADER_INSTALL_LIST}
 DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/foedag/InteractivePathAnalysis)
