cmake_minimum_required(VERSION 3.15)

project(projnavigator LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
else()
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_RELEASE} -Werror")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Werror")
endif()

include (../../cmake/cmake_qt.txt)
include (../../cmake/cmake_tcl.txt)

include_directories(${PROJECT_SOURCE_DIR}/../../src ${CMAKE_CURRENT_BINARY_DIR}/../../include/)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../lib)

set (SRC_CPP_LIST
  sources_form.cpp
  create_design_dialog.cpp
  add_file_dialog.cpp
  add_file_form.cpp
  runs_form.cpp)

set (SRC_H_LIST
  sources_form.h
  create_design_dialog.h
  add_file_dialog.h
  add_file_form.h
  runs_form.h)

set (SRC_UI_LIST
  sources_form.ui
  create_design_dialog.ui
  add_file_dialog.ui
  add_file_form.ui)

add_library(projnavigator STATIC
  ${SRC_CPP_LIST}
  ${SRC_H_LIST}
  ${SRC_UI_LIST}
)

target_link_libraries(projnavigator  PUBLIC Qt5::Widgets Qt5::Core Qt5::Gui Qt5::Xml)
target_compile_definitions(projnavigator PRIVATE PROJECTNAVIGATOR_LIBRARY)

set(PROJNAVI_STATIC_LIB libprojnavigator.a)
if (MSVC)
  set(PROJNAVI_STATIC_LIB projnavigator.lib)
endif()

install (
  FILES ${CMAKE_CURRENT_BINARY_DIR}/../../lib/${PROJNAVI_STATIC_LIB}
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/foedag)

install(
    FILES ${PROJECT_SOURCE_DIR}/../ProjNavigator/sources_form.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/foedag/ProjNavigator)
  
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../bin)

add_executable(projnavigator_bin
 ${PROJECT_SOURCE_DIR}/../ProjNavigator/Test/projnavigator_main.cpp
 #${PROJECT_SOURCE_DIR}/../ProjNavigator/projnavigator.qrc
 )
target_link_libraries(projnavigator_bin foedag tcl_stubb tcl_static zlib)
set_target_properties(projnavigator_bin PROPERTIES OUTPUT_NAME projnavigator)

