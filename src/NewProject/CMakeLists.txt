cmake_minimum_required(VERSION 3.15)

project(newproject LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 COMPONENTS Core Gui Widgets Xml REQUIRED)
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

include_directories(${PROJECT_SOURCE_DIR}/../../src ${CMAKE_CURRENT_BINARY_DIR}/../../include/)


set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../lib)

file(GLOB SRC_CPP_LIST "*.cpp" "ProjectManager/*.cpp")
file(GLOB SRC_H_LIST "*.h" "ProjectManager/*.h")
file(GLOB SRC_UI_LIST "*.ui")

add_library(newproject STATIC
    ${SRC_CPP_LIST}
    ${SRC_H_LIST}
    ${SRC_UI_LIST}
    newproject.qrc
)

target_link_libraries(newproject  PUBLIC Qt5::Widgets Qt5::Core Qt5::Gui Qt5::Xml)
target_compile_definitions(newproject PRIVATE NEWPROJECT_LIBRARY)

set(NEWPRO_STATIC_LIB libnewproject.a)
if (MSVC)
  set(NEWPRO_STATIC_LIB newproject.lib)
endif()

install (
  FILES ${CMAKE_CURRENT_BINARY_DIR}/../../lib/${NEWPRO_STATIC_LIB}
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/foedag)
