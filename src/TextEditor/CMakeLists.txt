cmake_minimum_required(VERSION 3.15)

project(texteditor LANGUAGES CXX)

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

include_directories(
  ${PROJECT_SOURCE_DIR}/../../src
  ${CMAKE_CURRENT_BINARY_DIR}/../../include/)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../lib)

set (SRC_CPP_LIST
  text_editor.cpp
  text_editor_form.cpp
  search_dialog.cpp)
if (FOEDAG_WITH_MONACO_EDITOR)
  list(APPEND SRC_CPP_LIST monaco_editor.cpp cpp_endpoint.cpp)
else()
  list(APPEND SRC_CPP_LIST editor.cpp)
endif()

set (SRC_H_LIST
  text_editor.h
  text_editor_form.h
  search_dialog.h)
if (FOEDAG_WITH_MONACO_EDITOR)
  list(APPEND SRC_H_LIST monaco_editor.h cpp_endpoint.h)
else()
  list(APPEND SRC_H_LIST editor.h)
endif()

set (SRC_UI_LIST
  )

add_library(texteditor STATIC
  ${SRC_CPP_LIST}
  ${SRC_H_LIST}
  ${SRC_UI_LIST}
)

if (FOEDAG_WITH_MONACO_EDITOR)
  target_link_libraries(texteditor  PUBLIC Qt6::Widgets Qt6::Core Qt6::Gui Qt6::WebEngineWidgets Qt6::WebChannel)
else()
  target_link_libraries(texteditor  PUBLIC Qt6::Widgets Qt6::Core Qt6::Gui)
endif()

target_compile_definitions(texteditor PRIVATE TEXTEDITOR_LIBRARY)

if (FOEDAG_WITH_MONACO_EDITOR)
  target_compile_definitions(texteditor PUBLIC FOEDAG_WITH_MONACO_EDITOR=${FOEDAG_WITH_MONACO_EDITOR})
endif()

set(TEXTEDITOR_STATIC_LIB libtexteditor.a)
if (MSVC)
	set(TEXTEDITOR_STATIC_LIB texteditor.lib)
endif()

install (
	FILES ${CMAKE_CURRENT_BINARY_DIR}/../../lib/${TEXTEDITOR_STATIC_LIB}
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/foedag)

install(
    FILES ${PROJECT_SOURCE_DIR}/../TextEditor/text_editor.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/foedag/TextEditor)
  
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../bin)

add_executable(texteditor_bin
 ${PROJECT_SOURCE_DIR}/../TextEditor/Test/texteditor_main.cpp
 )
target_link_libraries(texteditor_bin foedag tcl_stubb tcl_static zlib)
set_target_properties(texteditor_bin PROPERTIES OUTPUT_NAME texteditor)

