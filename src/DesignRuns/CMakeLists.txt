cmake_minimum_required(VERSION 3.15)

project(designruns LANGUAGES CXX)

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
 runs_form.cpp)

set (SRC_H_LIST
 runs_form.h)

set (SRC_UI_LIST
	)

add_library(designruns STATIC
  ${SRC_CPP_LIST}
  ${SRC_H_LIST}
  ${SRC_UI_LIST}
)

target_link_libraries(designruns  PUBLIC Qt5::Widgets Qt5::Core Qt5::Gui)
target_compile_definitions(designruns PRIVATE DESIGNRUNS_LIBRARY)

set(DESIGNRUNS_STATIC_LIB libdesignruns.a)
if (MSVC)
 set(DESIGNRUNS_STATIC_LIB designruns.lib)
endif()

install (
 FILES ${CMAKE_CURRENT_BINARY_DIR}/../../lib/${DESIGNRUNS_STATIC_LIB}
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/foedag)

install(
    FILES ${PROJECT_SOURCE_DIR}/../DesignRuns/runs_form.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/foedag/DesignRuns)
  
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../bin)

#add_executable(projnavigator_bin
# ${PROJECT_SOURCE_DIR}/../ProjNavigator/Test/projnavigator_main.cpp
 #${PROJECT_SOURCE_DIR}/../ProjNavigator/projnavigator.qrc
 # )
 #target_link_libraries(projnavigator_bin foedag tcl_stubb tcl_static zlib)
 #set_target_properties(projnavigator_bin PROPERTIES OUTPUT_NAME projnavigator)

