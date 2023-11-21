cmake_minimum_required(VERSION 3.15)

project(newfile LANGUAGES CXX)

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

include_directories(${PROJECT_SOURCE_DIR}/../../src ${CMAKE_CURRENT_BINARY_DIR}/../../include/)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../lib)

set (SRC_CPP_LIST
  new_file.cpp
  new_file_dialog.cpp
  newfilemodel.cpp)

set (SRC_H_LIST
  new_file.h
  new_file_dialog.h
  newfilemodel.h)

set (SRC_UI_LIST
	)
add_library(newfile STATIC
  ${SRC_CPP_LIST}
  ${SRC_H_LIST}
  ${SRC_UI_LIST}
)

target_link_libraries(newfile  PUBLIC Qt6::Widgets Qt6::Core Qt6::Gui)
target_compile_definitions(newfile PRIVATE NEWFILE_LIBRARY)

set(NEWFILE_STATIC_LIB libnewfile.a)
if (MSVC)
  set(NEWFILE_STATIC_LIB newfile.lib)
endif()

install (
  FILES ${CMAKE_CURRENT_BINARY_DIR}/../../lib/${NEWFILE_STATIC_LIB}
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/foedag)

install(
 FILES ${PROJECT_SOURCE_DIR}/../NewFile/new_file.h
 FILES ${PROJECT_SOURCE_DIR}/../NewFile/newfilemodel.h
 DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/foedag/NewFile)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../bin)

add_executable(newfile_bin
 ${PROJECT_SOURCE_DIR}/../NewFile/Test/newfile_main.cpp
 ${PROJECT_SOURCE_DIR}/../NewFile/Test/newfile_commands.cpp)
target_link_libraries(newfile_bin foedag tcl_stubb tcl_static zlib tclutils)
set_target_properties(newfile_bin PROPERTIES OUTPUT_NAME newfile)

