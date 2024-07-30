cmake_minimum_required(VERSION 3.15)

project(newproject LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
else()
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Werror")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Werror")
endif()

include (../../cmake/cmake_qt.txt)
include (../../cmake/cmake_tcl.txt)

include_directories(${PROJECT_SOURCE_DIR}/../../src ${PROJECT_SOURCE_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/../../include/)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../lib)

set (SRC_CPP_LIST
  #new_project_dialog.cpp
  new_project_dialog_ql.cpp
  location_form.cpp
  project_type_form.cpp
  add_source_form.cpp
  add_constraints_form.cpp
  device_planner_form.cpp
  summary_form.cpp
  create_file_dialog.cpp
  source_grid.cpp
  Main/registerNewProjectCommands.cpp
  ProjectManager/config.cpp
  ProjectManager/project_configuration.cpp
  ProjectManager/project_fileset.cpp
  ProjectManager/project_option.cpp
  ProjectManager/project_run.cpp
  ProjectManager/project.cpp
  ProjectManager/project_manager.cpp
  ProjectManager/compiler_configuration.cpp
  ProjectManager/ip_configuration.cpp
  ProjectManager/DesignFileWatcher.cpp
  ProjectManager/TaskStatusWatcher.cpp
  newprojectmodel.cpp
  add_sim_form.cpp
)

set (SRC_H_LIST
  new_project_dialog.h
  location_form.h
  project_type_form.h
  add_source_form.h
  add_constraints_form.h
  device_planner_form.h
  summary_form.h
  create_file_dialog.h
  source_grid.h
  ProjectManager/config.h
  Main/registerNewProjectCommands.h
  ProjectManager/project_configuration.h
  ProjectManager/project_fileset.h
  ProjectManager/project_option.h
  ProjectManager/project_run.h
  ProjectManager/project.h
  ProjectManager/project_manager.h
  ProjectManager/compiler_configuration.h
  ProjectManager/ip_configuration.h
  ProjectManager/DesignFileWatcher.h
  ProjectManager/TaskStatusWatcher.h
  newprojectmodel.h
  SettingsGuiInterface.h
  add_sim_form.h
)

set (SRC_UI_LIST
  new_project_dialog.ui
  location_form.ui
  project_type_form.ui
  add_source_form.ui
  add_constraints_form.ui
  device_planner_form.ui
  summary_form.ui
  create_file_dialog.ui
  add_sim_form.ui
)

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

install(
    FILES ${PROJECT_SOURCE_DIR}/../NewProject/new_project_dialog.h
    FILES ${PROJECT_SOURCE_DIR}/../NewProject/newprojectmodel.h
    FILES ${PROJECT_SOURCE_DIR}/../NewProject/SettingsGuiInterface.h
    FILES ${PROJECT_SOURCE_DIR}/../NewProject/add_constraints_form.h
    FILES ${PROJECT_SOURCE_DIR}/../NewProject/source_grid.h
    FILES ${PROJECT_SOURCE_DIR}/../NewProject/add_source_form.h
    FILES ${PROJECT_SOURCE_DIR}/../NewProject/add_sim_form.h
    FILES ${PROJECT_SOURCE_DIR}/../NewProject/device_planner_form.h
    FILES ${PROJECT_SOURCE_DIR}/../NewProject/location_form.h
    FILES ${PROJECT_SOURCE_DIR}/../NewProject/project_type_form.h
    FILES ${PROJECT_SOURCE_DIR}/../NewProject/summary_form.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/foedag/NewProject)


install(
      FILES ${PROJECT_SOURCE_DIR}/../NewProject/Main/registerNewProjectCommands.h
      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/foedag/NewProject/Main)

install(
      FILES ${PROJECT_SOURCE_DIR}/../NewProject/ProjectManager/project_manager.h
      FILES ${PROJECT_SOURCE_DIR}/../NewProject/ProjectManager/project.h
      FILES ${PROJECT_SOURCE_DIR}/../NewProject/ProjectManager/project_configuration.h
      FILES ${PROJECT_SOURCE_DIR}/../NewProject/ProjectManager/project_option.h
      FILES ${PROJECT_SOURCE_DIR}/../NewProject/ProjectManager/project_fileset.h
      FILES ${PROJECT_SOURCE_DIR}/../NewProject/ProjectManager/project_run.h
      FILES ${PROJECT_SOURCE_DIR}/../NewProject/ProjectManager/compiler_configuration.h
      FILES ${PROJECT_SOURCE_DIR}/../NewProject/ProjectManager/ip_configuration.h
      FILES ${PROJECT_SOURCE_DIR}/../NewProject/ProjectManager/DesignFileWatcher.h
      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/foedag/NewProject/ProjectManager)
  
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../bin)

add_executable(newproject_bin
	${PROJECT_SOURCE_DIR}/../NewProject/Main/newproject_main.cpp
	${PROJECT_SOURCE_DIR}/../NewProject/newproject.qrc)
target_link_libraries(newproject_bin foedag tcl_stubb tcl_static zlib)
set_target_properties(newproject_bin PROPERTIES OUTPUT_NAME newproject)

