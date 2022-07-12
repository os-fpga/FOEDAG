# -*- mode:cmake -*-

# Copyright 2021-2022 The Foedag team

# GPL License

# Copyright (c) 2021-2022 The Open-Source FPGA Foundation

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.


cmake_minimum_required(VERSION 3.15)

# Detect build type, fallback to release and throw a warning if use didn't
# specify any
if(NOT CMAKE_BUILD_TYPE)
  message(WARNING "Build type not set, falling back to Release mode.
 To specify build type use:
 -DCMAKE_BUILD_TYPE=<mode> where <mode> is Debug or Release.")
  set(CMAKE_BUILD_TYPE
      "Release"
      CACHE STRING "Choose the type of build, options are: Debug Release."
            FORCE)
endif(NOT CMAKE_BUILD_TYPE)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 0)
set(VERSION_PATCH 7)

option(
  WITH_LIBCXX
  "Building with clang++ and libc++(in Linux). To enable with: -DWITH_LIBCXX=On"
  On)

project(FOEDAG)

# Check system 
message("CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")

# Find Qt packages and error out if not found
find_package(Qt5 COMPONENTS Core Gui Widgets Quick Xml REQUIRED)
if (NOT Qt5Widgets_FOUND)
    message(STATUS "Failed to find Qt5Widgets required (on debian/ubuntu try 'sudo apt install qt5-default')")
elseif (NOT Qt5Gui_FOUND)
    message(STATUS "Failed to find Qt5Gui required (on debian/ubuntu try 'sudo apt install qt5-default')")
elseif (NOT Qt5Core_FOUND)
    message(STATUS "Failed to find Qt5Core required (on debian/ubuntu try 'sudo apt install qt5-default')")
elseif (NOT Qt5Quick_FOUND)
    message(STATUS "Failed to find Qt5Core required (on debian/ubuntu try 'sudo apt install libkf5qqc2desktopstyle-dev')")
else()
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTORCC ON)
endif()

# NOTE: Policy changes has to happen before adding any subprojects
cmake_policy(SET CMP0091 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_VERBOSE_MAKEFILE ON)

add_subdirectory(third_party/tcl_cmake EXCLUDE_FROM_ALL)
add_subdirectory(third_party/googletest EXCLUDE_FROM_ALL)
add_subdirectory(third_party/QScintilla-2.13.1)
add_subdirectory(third_party/QConsole)
add_subdirectory(third_party/nlohmann_json)
add_subdirectory(third_party/cryptor-gh)
add_subdirectory(tests/tclutils)
add_subdirectory(src/NewProject)
add_subdirectory(src/NewFile)
add_subdirectory(src/ProjNavigator)
add_subdirectory(src/DesignRuns)
add_subdirectory(src/Console)
add_subdirectory(src/Main)
add_subdirectory(src/Compiler)
add_subdirectory(src/IPGenerate)
add_subdirectory(src/TextEditor)


# NOTE: Set the global output directories after the subprojects have had their go at it
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

# Python
if (FOEDAG_WITH_PYTHON)
find_package(Python3 3.3 REQUIRED COMPONENTS Interpreter Development)
find_package(SWIG 3.0 REQUIRED)
message(STATUS "Python3_LIBRARIES = ${Python3_LIBRARIES}")
message(STATUS "Python3_EXECUTABLE = ${Python3_EXECUTABLE}")
message(STATUS "Python3_INCLUDE_DIRS = ${Python3_INCLUDE_DIRS}")
message(STATUS "Python3_RUNTIME_LIBRARY_DIRS = ${Python3_RUNTIME_LIBRARY_DIRS}")
endif()

if(NOT NO_TCMALLOC)
  find_library(TCMALLOC_LIBRARY NAMES tcmalloc)
  if(TCMALLOC_LIBRARY)
    set(TCMALLOC_COMPILE_OPTIONS
        "-fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free"
    )
  endif()
endif()

set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} ${TCMALLOC_COMPILE_OPTIONS} ${MY_CXX_WARNING_FLAGS}")

if(MSVC)
  add_subdirectory(third_party/zlib) # Do not use EXCLUDE_FROM_ALL else unit tests will fail for this subsystem

  add_compile_definitions(_CRT_NONSTDC_NO_WARNINGS)

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
  if(DEFINED ENV{MSYSTEM})
    # Under MSYS some files are too large to build without additional flags
    set(MSYS_COMPILE_OPTIONS "-m64 -Wa,-mbig-obj")
  endif()

  if (SANITIZE)
    set(MEM_SANITIZER_FLAGS "-fsanitize=address -fno-omit-frame-pointer")
  endif()

  set(CMAKE_CXX_FLAGS_DEBUG
      "${CMAKE_CXX_FLAGS_DEBUG} ${TCMALLOC_COMPILE_OPTIONS} -Werror -Wall -O0 -g ${MSYS_COMPILE_OPTIONS} ${MY_CXX_WARNING_FLAGS} ${MEM_SANITIZER_FLAGS}"
  )
  set(CMAKE_CXX_FLAGS_RELEASE
      "${CMAKE_CXX_FLAGS_RELEASE} ${TCMALLOC_COMPILE_OPTIONS} -Werror -Wall -O3 ${MSYS_COMPILE_OPTIONS} -DNDEBUG ${MY_CXX_WARNING_FLAGS}"
  )
endif()

include_directories(${PROJECT_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR}/include/)

set(QRC_MAIN_WINDOW 
	src/MainWindow/main_window_resource.qrc
        src/MainWindow/qml.qrc
	${PROJECT_SOURCE_DIR}/src/NewProject/newproject.qrc)

# Put source code here, files that are generated at build time in
# foedag_generated_SRC
set(foedag_SRC  
  ${QRC_MAIN_WINDOW}
)

add_library(foedag STATIC ${foedag_SRC})

set_target_properties(foedag PROPERTIES PUBLIC_HEADER src/Main/Foedag.h)
target_include_directories(foedag PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}/include
  third_party/googletest/googletest/include
  third_party/googletest/googlemock/include)
target_include_directories(foedag PUBLIC $<INSTALL_INTERFACE:include/foedag>)

if(WIN32)
  if(MSVC)
    #add_executable(foedag-bin ${PROJECT_SOURCE_DIR}/src/Main/main.cpp ${QRC_MAIN_WINDOW})
    add_executable(foedag-bin ${PROJECT_SOURCE_DIR}/src/Main/main_ql.cpp ${QRC_MAIN_WINDOW})
    set_property(TARGET foedag-bin PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set_property(TARGET foedag-bin PROPERTY COMPILER_FLAGS /DSTATIC_BUILD)
  elseif((DEFINED ENV{MSYSTEM}) AND ("$ENV{MSYSTEM}" STREQUAL "MINGW64"))
    # use this for a 'console' application on Windows - ensure that FreeConsole() is called to dismiss the console in GUI mode
    add_executable(foedag-bin ${PROJECT_SOURCE_DIR}/src/Main/main_ql.cpp ${QRC_MAIN_WINDOW})
    # use this for a 'gui' application on Windows - however, console output is not seen (TODO: we can bind to console if needed)
    #add_executable(foedag-bin WIN32 ${PROJECT_SOURCE_DIR}/src/Main/main_ql.cpp ${QRC_MAIN_WINDOW})
  endif(MSVC)
else(WIN32)
  add_executable(foedag-bin ${PROJECT_SOURCE_DIR}/src/Main/main_ql.cpp ${QRC_MAIN_WINDOW})
endif(WIN32)
set_target_properties(foedag-bin PROPERTIES OUTPUT_NAME aurora)

if (MSVC)
  message("WINDOWS MODE")
  set(TCL_STUBB_LIB tclstub86.lib)
  set(TCL_STATIC_LIB tcl86ts.lib)
  set(ZLIB_STATIC_LIB zlib.lib)

  add_library(tcl_static STATIC IMPORTED )
  set_target_properties(tcl_static PROPERTIES
    IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/lib/${TCL_STATIC_LIB})

else(MSVC)
  set(TCL_STATIC_LIB libtcl8.6.so)
  set(TCL_STUBB_LIB libtclstub8.6.a)
  set(ZLIB_STATIC_LIB libz.a)
  if(APPLE) 
    set(TCL_STATIC_LIB libtcl8.6.dylib)
  endif()
  
  link_directories(${CMAKE_CURRENT_BINARY_DIR}/lib/)

  if((DEFINED ENV{MSYSTEM}) AND ("$ENV{MSYSTEM}" STREQUAL "MINGW64"))
    message("MSYS MODE")
    set(TCL_STATIC_LIB libtcl86.dll.a)
    set(TCL_STUBB_LIB libtclstub86.a)
    set(ZLIB_STATIC_LIB libzlibstatic.a)

    add_library(tcl_static STATIC IMPORTED )
    set_target_properties(tcl_static PROPERTIES
      IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/lib/${TCL_STATIC_LIB})
  else((DEFINED ENV{MSYSTEM}) AND ("$ENV{MSYSTEM}" STREQUAL "MINGW64"))
    message("LINUX MODE")
    get_filename_component(buildDirRelFilePath ${TCL_STATIC_LIB}
                         REALPATH BASE_DIR ${CMAKE_CURRENT_BINARY_DIR}/lib)
  
    add_library(tcl_static SHARED IMPORTED )
    find_library(tcl_static PATHS ${CMAKE_CURRENT_BINARY_DIR}/lib/)
    set_target_properties(tcl_static PROPERTIES
      IMPORTED_LOCATION ${buildDirRelFilePath} IMPORTED_NO_SONAME ON)

  endif((DEFINED ENV{MSYSTEM}) AND ("$ENV{MSYSTEM}" STREQUAL "MINGW64"))
  
endif(MSVC)

add_library(tcl_stubb STATIC IMPORTED )
set_target_properties(tcl_stubb PROPERTIES
  IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/lib/${TCL_STUBB_LIB})

if(MSVC)
  set_target_properties(tcl_static PROPERTIES
    COMPILE_OPTIONS "$<$<CONFIG:Debug>:/MTd>$<$<CONFIG:Release>:/MT>"
  )

  set_target_properties(tcl_stubb PROPERTIES
    COMPILE_OPTIONS "$<$<CONFIG:Debug>:/MTd>$<$<CONFIG:Release>:/MT>"
  )

  # Do not add zlib library, it is imported above

  set_target_properties(foedag PROPERTIES
    COMPILE_OPTIONS "$<$<CONFIG:Debug>:/MTd>$<$<CONFIG:Release>:/MT>"
  )
  target_link_libraries(foedag-bin PUBLIC Netapi32)
  target_link_libraries(foedag  PUBLIC Netapi32)

else()
  add_library(zlib STATIC IMPORTED)
  set_target_properties(zlib PROPERTIES
    IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/lib/${ZLIB_STATIC_LIB})       
endif()

# Copy the init.tcl file from source to build directory
add_custom_command(TARGET foedag POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy 
          ${CMAKE_CURRENT_SOURCE_DIR}/third_party/tcl8.6.12/library/init.tcl
          ${CMAKE_CURRENT_BINARY_DIR}/lib/tcl8.6/init.tcl)

execute_process(COMMAND git rev-parse --short HEAD
  OUTPUT_VARIABLE GIT_HASH
  ERROR_VARIABLE GIT_HASH_ERR
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

message(STATUS "GIT_HASH = ${GIT_HASH}")
configure_file(foedag_version.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/foedag_version.h)

# Explicit lib build order
add_dependencies(foedag foedagcore)
add_dependencies(compiler tcl_stubb_build)
add_dependencies(ipgenerate tcl_stubb_build)
add_dependencies(foedagcore tcl_stubb_build)
add_dependencies(console tcl_stubb_build)
add_dependencies(console tcl_static)
add_dependencies(newproject tcl_stubb_build)
add_dependencies(tcl_stubb_build tcl_build)
add_dependencies(console QConsole)
add_dependencies(tclutils tcl_stubb_build)
if(MSVC)
else()
  add_dependencies(tcl_build zlib_build)
endif()
add_dependencies(foedagcore qscintilla2_qt)
add_dependencies(texteditor qscintilla2_qt tcl_build)
add_dependencies(designruns tcl_build)

add_dependencies(tcl_static tcl_build)
add_dependencies(foedag-bin tcl_build)
add_dependencies(designruns_bin tcl_build)
add_dependencies(foedagcore tcl_build)
add_dependencies(console_debug tcl_build)
add_dependencies(newfile tcl_stubb_build)
add_dependencies(newfile_bin tcl_stubb_build)
add_dependencies(compiler cryptor)

if (APPLE)
  # In macOS, it is necessary to add the correct @rpath to the executable for
  # finding python dynamic libraries ref: https://gitlab.kitware.com/cmake/cmake/-/issues/21293
  # https://gitlab.kitware.com/cmake/cmake/-/issues/21947
  # Python3_LINK_OPTIONS is variable available from cmake 3.19, update cmake using homebrew
  # if can't update cmake use:
  # set_target_properties(foedag-bin PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE
  # INSTALL_RPATH "/Library/Developer/CommandLineTools/Library/Frameworks/")
  # if you installed python with hombrew. Or if you install python with Xcode:
  # set_target_properties(foedag-bin PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE
  # INSTALL_RPATH "/Applications/Xcode.app/Contents/Developer/Library/Frameworks/")
  target_link_libraries(foedag-bin PUBLIC foedag "-framework CoreFoundation")
  target_link_libraries(foedag PUBLIC "-framework CoreFoundation")
endif()

if(MSVC OR WIN32)
  # We have two files named "foedag.lib" and both getting generated in the lib folder
  # One is the foedag.lib generated by the foedag target and the other is the one generated
  # because of /IMPLIB option when linking the executable. Unfortunately, there is no documented
  # way to disable the latter in CMake. So, moving the library to the bin directory (right next to the exe)
  set_target_properties(foedag-bin PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
endif()

target_link_libraries(foedag-bin PUBLIC foedag )
target_link_directories(foedag-bin PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/lib PUBLIC ${CMAKE_INSTALL_PREFIX}/lib/foedag/lib)
target_link_libraries(foedag  PUBLIC foedagcore newproject newfile projnavigator designruns texteditor qscintilla2_qt compiler ipgenerate console QConsole tcl_stubb tcl_static zlib nlohmann_json)
target_link_libraries(foedag  PUBLIC Qt5::Widgets Qt5::Core Qt5::Gui Qt5::Xml Qt5::Quick)

if(NOT NO_TCMALLOC)
  find_library(TCMALLOC_LIBRARY NAMES tcmalloc)
  if(TCMALLOC_LIBRARY)
    target_link_libraries(foedag PRIVATE tcmalloc)
  endif()
endif()

if (UNIX)
  target_link_libraries(foedag PRIVATE dl)
  target_link_libraries(foedag PRIVATE util)
  target_link_libraries(foedag PRIVATE m)
  target_link_libraries(foedag PRIVATE pthread)
endif()

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
  target_link_libraries(foedag PRIVATE stdc++fs)
  target_link_libraries(foedag PRIVATE rt)
endif()

# Unit tests
enable_testing()
include(GoogleTest)

if(MSVC)
  # Microsoft reports the value of __cplusplus wrong and gmock/gtest pulls in the
  # string_view implementation based on it's value. Microsoft's solution is to
  # provide additional flags to make the value correct. More info can be found here -
  #
  #   https://docs.microsoft.com/en-us/cpp/build/reference/zc-cplusplus?view=msvc-160
  #   https://devblogs.microsoft.com/cppblog/msvc-now-correctly-reports-__cplusplus/
  target_compile_options(gmock PRIVATE /Zc:__cplusplus)
  target_compile_options(gmock_main PRIVATE /Zc:__cplusplus)
  target_compile_options(gtest PRIVATE /Zc:__cplusplus)
  target_compile_options(gtest_main PRIVATE /Zc:__cplusplus)
endif()


# All unit-tests are registered with this custom target as dependency, so
# just `make UnitTests` will build them all.
add_custom_target(UnitTests)

# Concise way to register a new google test
# call with register_gtest(path/to/UnitTestFile.cpp)
function(register_gtests)
  foreach(gtest_cc_file IN LISTS ARGN)
    # We create the binary name and test prefix from the cpp-filepath
    get_filename_component(test_bin ${gtest_cc_file} NAME_WE)
    get_filename_component(test_prefix ${gtest_cc_file} DIRECTORY)

    # Build binary, link all relevant libs and extract tests
    add_executable(${test_bin} EXCLUDE_FROM_ALL ${gtest_cc_file})

    target_include_directories(${test_bin} PRIVATE
    )
    # For simplicity, we link the test with libfoedag, but there is of
    # course a lot unnecessary churn if headers are modified.
    # Often it is sufficient to just have a few depeendencies.

    if (APPLE)
      target_link_libraries(${test_bin} foedag gtest gmock gtest_main "-framework CoreFoundation")
    elseif (CMAKE_SYSTEM_NAME MATCHES "Darwin")
      target_link_libraries(${test_bin} foedag gtest gmock gtest_main "-framework CoreFoundation")
    else()
      target_link_libraries(${test_bin} foedag gtest gmock gtest_main)
    endif()

    gtest_discover_tests(${test_bin} TEST_PREFIX "${test_prefix}/")

    # Now, add this binary to our UnitTests target that it builds this
    add_dependencies(UnitTests ${test_bin})
  endforeach()
endfunction()

register_gtests(
  src/Tcl/HelloTcl_test.cpp
  src/Command/Command_test.cpp
)

if (WIN OR APPLE)
else ()
# The test works, the CI running headlessly does not
#  register_gtests(src/Main/GuiMain_test.cpp)
endif()

# Installation target
install(
  TARGETS foedag-bin
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(
  TARGETS foedag 
  EXPORT Foedag
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/foedag
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/foedag)

install (
  FILES ${CMAKE_CURRENT_BINARY_DIR}/lib/${TCL_STATIC_LIB}
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/foedag/lib)
install (
  FILES ${CMAKE_CURRENT_BINARY_DIR}/lib/${TCL_STUBB_LIB}
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/foedag/lib)
install (
  FILES ${CMAKE_CURRENT_BINARY_DIR}/lib/${ZLIB_STATIC_LIB}
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/foedag/lib)  
  
  
if (WIN32 AND $<CONFIG:Debug>)
  if (FOEDAG_WITH_PYTHON)
    install(
      FILES $<TARGET_PDB_FILE:foedag-bin>
            ${Python3_RUNTIME_LIBRARY_DIRS}/python${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR}$<$<CONFIG:Debug>:_d>.dll
      DESTINATION ${CMAKE_INSTALL_BINDIR})
  endif()
  install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/foedag.dir/foedag.pdb
          ${TCL_BINARY_DIR}/runtime/CMakeFiles/tcl_static.dir/tcl_static.pdb
          ${TCL_BINARY_DIR}/runtime/CMakeFiles/tcl_stubb.dir/tcl_stubb.pdb
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/foedag)
endif()

if ((DEFINED ENV{MSYSTEM}) AND ("$ENV{MSYSTEM}" STREQUAL "MINGW64"))
# we do not have the TCL header files to 'install' as we don't build it.
# we just use the TCL MinGW64 lib.
else((DEFINED ENV{MSYSTEM}) AND ("$ENV{MSYSTEM}" STREQUAL "MINGW64"))
install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/include/tcl.h
        ${CMAKE_CURRENT_BINARY_DIR}/include/tclDecls.h
        ${CMAKE_CURRENT_BINARY_DIR}/include/tclPlatDecls.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/foedag/include)
endif((DEFINED ENV{MSYSTEM}) AND ("$ENV{MSYSTEM}" STREQUAL "MINGW64"))
# install(
#  EXPORT Foedag
#  FILE Foedag.cmake
#  DESTINATION cmake)
include(CMakePackageConfigHelpers)

# generate the config file that is includes the exports
configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/FoedagConfig.cmake"
  INSTALL_DESTINATION cmake
  NO_SET_AND_CHECK_MACRO
  NO_CHECK_REQUIRED_COMPONENTS_MACRO)

# install the configuration file
install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/FoedagConfig.cmake
  DESTINATION cmake)

install(
  FILES etc/device.xml DESTINATION  ${CMAKE_INSTALL_DATAROOTDIR}/foedag/etc/)
  
install(
  FILES etc/settings/settings_test.json DESTINATION  ${CMAKE_INSTALL_DATAROOTDIR}/foedag/etc/settings/)

install(
  DIRECTORY examples DESTINATION  ${CMAKE_INSTALL_DATAROOTDIR}/foedag)
install(
    DIRECTORY tests/Arch/ DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/foedag/Arch/)
    
# add_custom_target(link_target ALL
#                   COMMAND ${CMAKE_COMMAND} -E create_symlink
#                   build/compile_commands.json ../compile_commands.json)

add_custom_command(TARGET foedag-bin POST_BUILD
                  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/share/foedag/etc/
                  COMMAND ${CMAKE_COMMAND} -E copy 
                  ${PROJECT_SOURCE_DIR}/etc/device.xml        
                          ${CMAKE_CURRENT_BINARY_DIR}/share/foedag/etc/)
                          
add_custom_command(TARGET foedag-bin POST_BUILD
                  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/share/foedag/etc/settings/
                  COMMAND ${CMAKE_COMMAND} -E copy 
                  ${PROJECT_SOURCE_DIR}/etc/settings/settings_test.json
                  ${CMAKE_CURRENT_BINARY_DIR}/share/foedag/etc/settings/)

add_custom_command(TARGET foedag-bin POST_BUILD
                  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/share/foedag/examples/
                  COMMAND ${CMAKE_COMMAND} -E copy_directory
                  ${PROJECT_SOURCE_DIR}/examples
                  ${CMAKE_CURRENT_BINARY_DIR}/share/foedag/examples/)

add_custom_command(TARGET foedag-bin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/share/foedag/Arch/
    COMMAND ${CMAKE_COMMAND} -E copy_directory 
        ${PROJECT_SOURCE_DIR}/tests/Arch  
        ${CMAKE_CURRENT_BINARY_DIR}/share/foedag/Arch/)
