# -*- mode:cmake -*-
cmake_minimum_required(VERSION 3.15)

# Usage:
# After a make install at the top drectory:
# cd tests/TestInstall;
# rm -rf build;
# mkdir -p build;
# cd build;
# cmake ../ -DINSTALL_DIR=/usr/local;
# make
# ./foedag --version

project(HELLOFOEDAG)

# NOTE: Policy changes has to happen before adding any subprojects
cmake_policy(SET CMP0091 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")


find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)
if (NOT Qt5Widgets_FOUND)
    message(STATUS "Failed to find Qt5Widgets required (on debian/ubuntu try 'sudo apt install qt5-default')")
elseif (NOT Qt5Gui_FOUND)
    message(STATUS "Failed to find Qt5Gui required (on debian/ubuntu try 'sudo apt install qt5-default')")
elseif (NOT Qt5Core_FOUND)
    message(STATUS "Failed to find Qt5Core required (on debian/ubuntu try 'sudo apt install qt5-default')")
else()
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTORCC ON)
endif()


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Python
if (FOEDAG_WITH_PYTHON)
find_package(Python3 3.3 REQUIRED COMPONENTS Interpreter Development)
message(STATUS "Python3_LIBRARIES = ${Python3_LIBRARIES}")
message(STATUS "Python3_EXECUTABLE = ${Python3_EXECUTABLE}")
message(STATUS "Python3_INCLUDE_DIRS = ${Python3_INCLUDE_DIRS}")
message(STATUS "Python3_RUNTIME_LIBRARY_DIRS = ${Python3_RUNTIME_LIBRARY_DIRS}")
endif()

set(TCL_STATIC_LIB libtcl9.0.a)
set(TCL_STUBB_LIB libtclstub9.0.a)
set(ZLIB_STATIC_LIB libz.a)
if (MSVC)
  set(TCL_STUBB_LIB tclstub90.lib)
  set(TCL_STATIC_LIB tcl90s.lib)
  set(ZLIB_STATIC_LIB zlib.lib)
endif()

ADD_LIBRARY(tcl_static STATIC IMPORTED)
ADD_LIBRARY(tcl_stubb STATIC IMPORTED)

SET_TARGET_PROPERTIES(tcl_static  PROPERTIES
  IMPORTED_LOCATION ${INSTALL_DIR}/lib/foedag/lib/${TCL_STATIC_LIB})
SET_TARGET_PROPERTIES(tcl_stubb  PROPERTIES
  IMPORTED_LOCATION ${INSTALL_DIR}/lib/foedag/lib/${TCL_STUBB_LIB})   


ADD_LIBRARY(zlib STATIC IMPORTED)
SET_TARGET_PROPERTIES(zlib  PROPERTIES
    IMPORTED_LOCATION ${INSTALL_DIR}/lib/${ZLIB_STATIC_LIB})

if(NOT NO_TCMALLOC)
  find_library(TCMALLOC_LIBRARY NAMES tcmalloc)
endif()

# Directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${TCMALLOC_COMPILE_OPTIONS} ${MY_CXX_WARNING_FLAGS}")
if(MSVC)
  add_compile_definitions(_CRT_NONSTDC_NO_WARNINGS)
  set(CMAKE_CXX_FLAGS_DEBUG  "${CMAKE_CXX_FLAGS_DEBUG} ${TCMALLOC_COMPILE_OPTIONS} /Zc:__cplusplus /W0 /bigobj /GL- /DSTATIC_BUILD ${MY_CXX_WARNING_FLAGS}")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${TCMALLOC_COMPILE_OPTIONS} /Zc:__cplusplus /W0 /bigobj /GL- /DSTATIC_BUILD ${MY_CXX_WARNING_FLAGS}")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /STACK:8388608 /NODEFAULTLIB:library")  # 8MB stack size
else()
  # C++ Compilation line
  if($<BOOL:${TCMALLOC_LIBRARY}>)
    set (TCMALLOC_COMPILE_OPTIONS "-fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free")
  endif($<BOOL:${TCMALLOC_LIBRARY}>)

  set(CMAKE_CXX_FLAGS_DEBUG  "${CMAKE_CXX_FLAGS_DEBUG} ${TCMALLOC_COMPILE_OPTIONS} -Wall -O0 -g ${MY_CXX_WARNING_FLAGS}")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${TCMALLOC_COMPILE_OPTIONS} -Wall -O3 -DNDEBUG ${MY_CXX_WARNING_FLAGS}")
endif()

add_executable(test_hellofoedag ${PROJECT_SOURCE_DIR}/../../src/Main/main.cpp)

if(MSVC)
  set_property(TARGET test_hellofoedag PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
  set_property(TARGET test_hellofoedag PROPERTY COMPILER_FLAGS /DSTATIC_BUILD)
endif()

target_include_directories(test_hellofoedag PRIVATE
  ${INSTALL_DIR}/include/foedag
  ${INSTALL_DIR}/include
  ${INSTALL_DIR}/include/foedag/include
  ${Python3_INCLUDE_DIRS}
)
target_link_directories(test_hellofoedag
    PRIVATE ${INSTALL_DIR}/lib/foedag
)

if (FOEDAG_WITH_PYTHON)
  target_link_libraries(test_hellofoedag
    foedag
    tcl_stubb
    tcl_static
    zlib
    ${Python3_LIBRARIES}
    Qt5::Widgets Qt5::Core Qt5::Gui 
    $<$<BOOL:${TCMALLOC_LIBRARY}>:tcmalloc>
)
else()
  target_link_libraries(test_hellofoedag
    foedag
    tcl_stubb
    tcl_static
    zlib
    Qt5::Widgets Qt5::Core Qt5::Gui
    $<$<BOOL:${TCMALLOC_LIBRARY}>:tcmalloc>
)
endif()

if(MSVC)
  SET_TARGET_PROPERTIES(tcl_static PROPERTIES
    IMPORTED_LOCATION ${INSTALL_DIR}/lib/foedag/lib/${TCL_STATIC_LIB})
  set_target_properties(tcl_static PROPERTIES
    COMPILE_OPTIONS "/MT"
  )
  SET_TARGET_PROPERTIES(tcl_stubb PROPERTIES
    IMPORTED_LOCATION ${INSTALL_DIR}/lib/foedag/lib/${TCL_STUBB_LIB})
  set_target_properties(tcl_stubb PROPERTIES
    COMPILE_OPTIONS "/MT>"
  )
  set_target_properties(test_hellofoedag PROPERTIES
    COMPILE_OPTIONS "/MT"
  )
  target_link_libraries(test_hellofoedag Netapi32)
endif()

if (APPLE)
  target_link_libraries(test_hellofoedag
      dl
      util
      m
      pthread
      "-framework CoreFoundation"
  )
elseif (UNIX)
  target_link_libraries(test_hellofoedag
      stdc++fs
      dl
      util
      m
      rt
      pthread
  )
endif()

if (WIN32)
  if (FOEDAG_WITH_PYTHON)
    add_custom_command(
      TARGET test_hellofoedag
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${Python3_RUNTIME_LIBRARY_DIRS}/python${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR}$<$<CONFIG:Debug>:_d>.dll
            $<TARGET_FILE_DIR:test_hellosureworld>)
    endif()
endif()

add_custom_target(RunInstallTest ALL
    DEPENDS test_hellofoedag
    COMMAND $<TARGET_FILE:test_hellofoedag> -noqt
    WORKING_DIRECTORY $<TARGET_FILE_DIR:test_hellofoedag>)
