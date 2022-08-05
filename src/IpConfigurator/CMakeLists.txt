cmake_minimum_required(VERSION 3.15)

project(ipconfigurator LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
else()
  set(CMAKE_CXX_FLAGS_DEBUG
  "${CMAKE_CXX_FLAGS_DEBUG} ${TCMALLOC_COMPILE_OPTIONS} -Werror -Wall -O0 -g ${MSYS_COMPILE_OPTIONS} ${MY_CXX_WARNING_FLAGS} ${MEM_SANITIZER_FLAGS}")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Werror")
endif()

include (../../cmake/cmake_qt.txt)
include (../../cmake/cmake_tcl.txt)

include_directories(
  ${PROJECT_SOURCE_DIR}/../../src
  ${CMAKE_CURRENT_BINARY_DIR}/../../include/)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../lib)

set (SRC_CPP_LIST
  IpCatalogTree.cpp
  IpConfigDlg.cpp
  IpConfigurator.cpp
  IpInstancesTree.cpp
  IpTreesWidget.cpp
)

set (SRC_H_INSTALL_LIST
  IpCatalogTree.h
  IpConfigDlg.h
  IpConfigurator.h
  IpInstancesTree.h
  IpTreesWidget.h
)

set (SRC_H_LIST
    ${SRC_H_INSTALL_LIST}
)

set (SRC_UI_LIST
  )

add_library(ipconfigurator STATIC
  ${SRC_CPP_LIST}
  ${SRC_H_LIST}
  ${SRC_UI_LIST}
)

target_link_libraries(ipconfigurator PUBLIC Qt5::Widgets Qt5::Core Qt5::Gui)
target_compile_definitions(ipconfigurator PRIVATE IPCONFIGURATOR_LIBRARY)

install (
	FILES ${CMAKE_CURRENT_BINARY_DIR}/../../lib/$<TARGET_FILE_NAME:ipconfigurator>
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/foedag)

foreach(file ${SRC_H_INSTALL_LIST})
  install(
      FILES ${PROJECT_SOURCE_DIR}/../IpConfigurator/${file}
      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/foedag/IpConfigurator)
endforeach()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../bin)

add_executable(ipconfigurator_bin
 ${PROJECT_SOURCE_DIR}/../IpConfigurator/Test/ipconfigurator_main.cpp
 )
target_link_libraries(ipconfigurator_bin foedag tcl_stubb tcl_static zlib)
set_target_properties(ipconfigurator_bin PROPERTIES OUTPUT_NAME ipconfigurator)

