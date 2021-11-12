cmake_minimum_required(VERSION 3.15)

project(newproject LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
else()
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_RELEASE} -Wno-deprecated-declarations -Werror")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wno-deprecated-declarations -Werror")
endif()

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


include_directories(${PROJECT_SOURCE_DIR}/../../src ${CMAKE_CURRENT_BINARY_DIR}/../../include/)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../lib)

set (SRC_CPP_LIST
  new_project_dialog.cpp
  location_form.cpp
  project_type_form.cpp
  add_source_form.cpp
  add_constraints_form.cpp
  device_planner_form.cpp
  summary_form.cpp
  create_file_dialog.cpp
  source_grid.cpp
  ProjectManager/config.cpp
  ProjectManager/project_configuration.cpp
  ProjectManager/project_fileset.cpp
  ProjectManager/project_option.cpp
  ProjectManager/project_run.cpp)

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
  ProjectManager/project_configuration.h
  ProjectManager/project_fileset.h
  ProjectManager/project_option.cpp
  ProjectManager/project_run.cpp)

set (SRC_UI_LIST
  new_project_dialog.ui
  location_form.ui
  project_type_form.ui
  add_source_form.ui
  add_constraints_form.ui
  device_planner_form.ui
  summary_form.ui
  create_file_dialog.ui)

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
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/foedag/NewProject)
  
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../bin)

add_executable(newproject_bin
	${PROJECT_SOURCE_DIR}/../NewProject/Main/newproject_main.cpp
	${PROJECT_SOURCE_DIR}/../NewProject/newproject.qrc)
target_link_libraries(newproject_bin foedag tcl_stubb tcl_static zlib)
set_target_properties(newproject_bin PROPERTIES OUTPUT_NAME newproject)

