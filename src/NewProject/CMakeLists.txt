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

add_library(newproject STATIC
  add_constraints_form.cpp
  add_constraints_form.h
  add_constraints_form.ui
  add_source_form.cpp
  add_source_form.h
  add_source_form.ui
  device_planner_form.cpp
  device_planner_form.h
  device_planner_form.ui
  location_form.cpp
  location_form.h
  location_form.ui
  new_project_dialog.cpp
  new_project_dialog.h
  new_project_dialog.ui
  project_type_form.cpp
  project_type_form.h
  project_type_form.ui
  source_grid.cpp
  source_grid.h
  summary_form.cpp
  summary_form.h
  summary_form.ui
  create_file_dialog.cpp
  create_file_dialog.h
  create_file_dialog.ui
  newproject.qrc
)
target_link_libraries(newproject  PUBLIC Qt5::Widgets Qt5::Core Qt5::Gui Qt5::Xml)
target_compile_definitions(newproject PRIVATE NEWPROJECT_LIBRARY)

add_executable(newproject_bin
	${PROJECT_SOURCE_DIR}/newproject_main.cpp
	${PROJECT_SOURCE_DIR}/newproject.qrc)
target_link_libraries(newproject_bin foedag)
set_target_properties(newproject_bin PROPERTIES OUTPUT_NAME newproject)


set(NEWPRO_STATIC_LIB libnewproject.a)
if (MSVC)
  set(NEWPRO_STATIC_LIB newproject.lib)
endif()

install (
  FILES ${CMAKE_CURRENT_BINARY_DIR}/../../lib/${NEWPRO_STATIC_LIB}
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/foedag)
