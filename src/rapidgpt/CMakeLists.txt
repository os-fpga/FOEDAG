cmake_minimum_required(VERSION 3.15)

project(rapidgpt LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
else()
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Werror")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Werror")
endif()

include (../../cmake/cmake_qt.txt)

include_directories(${PROJECT_SOURCE_DIR}/../../src ${CMAKE_CURRENT_BINARY_DIR}/../../include/)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../lib)

set (SRC_CPP_LIST
  RapigGptSettingsWindow.cpp
  RapidGpt.cpp
  RapidGptContext.cpp
)

set (SRC_H_LIST
  RapigGptSettingsWindow.h
  RapidGpt.h
  RapidGptContext.h
)

set (SRC_UI_LIST
)

add_library(rapidgpt STATIC
  ${SRC_CPP_LIST}
  ${SRC_H_LIST}
  ${SRC_UI_LIST}
)

find_package(Qt6 COMPONENTS Network REQUIRED)
if (NOT Qt6Network_FOUND)
    message(STATUS "Failed to find Qt6Network")
endif()

target_link_libraries(rapidgpt  PUBLIC Qt6::Widgets Qt6::Core Qt6::Gui Qt6::Network)
target_compile_definitions(rapidgpt PRIVATE RAPIDGPR_LIBRARY)

install (
  FILES ${CMAKE_CURRENT_BINARY_DIR}/../../lib/$<TARGET_FILE_NAME:rapidgpt>
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/foedag)

set (HEADER_INSTALL_LIST "")
foreach(HEADER ${SRC_H_LIST})
  list(APPEND HEADER_INSTALL_LIST ${PROJECT_SOURCE_DIR}/../rapidgpt/${HEADER})
endforeach()

install(
 FILES ${HEADER_INSTALL_LIST}
 DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/foedag/rapidgpt)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../bin)
