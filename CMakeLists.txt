# -*- mode:cmake -*-
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

option(
  WITH_LIBCXX
  "Building with clang++ and libc++(in Linux). To enable with: -DWITH_LIBCXX=On"
  On)

project(FOEDAG)

# Check system 
message("CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")

# Find Qt packages and error out if not found
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

# NOTE: Policy changes has to happen before adding any subprojects
cmake_policy(SET CMP0091 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(third_party/tcl_cmake EXCLUDE_FROM_ALL)
add_subdirectory(third_party/zlib) # Do not use EXCLUDE_FROM_ALL else unit tests will fail for this subsystem
add_subdirectory(third_party/googletest EXCLUDE_FROM_ALL)

# NOTE: Set the global output directories after the subprojects have had their go at it
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

set(GENDIR ${CMAKE_CURRENT_BINARY_DIR}/generated)

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
  add_compile_definitions(_CRT_NONSTDC_NO_WARNINGS)

  set(CMAKE_CXX_FLAGS_DEBUG
      "${CMAKE_CXX_FLAGS_DEBUG} ${TCMALLOC_COMPILE_OPTIONS} /Zc:__cplusplus /W0 /bigobj ${MY_CXX_WARNING_FLAGS}"
  )
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO
      "${CMAKE_CXX_FLAGS_RELEASE} ${TCMALLOC_COMPILE_OPTIONS} /Zc:__cplusplus /W0 /bigobj ${MY_CXX_WARNING_FLAGS}"
  )
  set(CMAKE_CXX_FLAGS_RELEASE
      "${CMAKE_CXX_FLAGS_RELEASE} ${TCMALLOC_COMPILE_OPTIONS} /Zc:__cplusplus /W0 /bigobj ${MY_CXX_WARNING_FLAGS}"
  )
  set(CMAKE_EXE_LINKER_FLAGS /STACK:8388608)  # 8MB stack size
else()
  if(DEFINED ENV{MSYSTEM})
    # Under MSYS some files are too large to build without additional flags
    set(MSYS_COMPILE_OPTIONS "-m64 -Wa,-mbig-obj")
  endif()
  #set(MEM_SANITIZER_FLAGS
  #    "-fsanitize=address -fno-omit-frame-pointer"
  #)
  set(CMAKE_CXX_FLAGS_DEBUG
      "${CMAKE_CXX_FLAGS_DEBUG} ${TCMALLOC_COMPILE_OPTIONS} -Wall -O0 -g ${MSYS_COMPILE_OPTIONS} ${MY_CXX_WARNING_FLAGS} ${MEM_SANITIZER_FLAGS}"
  )
  set(CMAKE_CXX_FLAGS_RELEASE
      "${CMAKE_CXX_FLAGS_RELEASE} ${TCMALLOC_COMPILE_OPTIONS} -Wall -O3 ${MSYS_COMPILE_OPTIONS} -DNDEBUG ${MY_CXX_WARNING_FLAGS}"
  )
endif()

include_directories(${PROJECT_SOURCE_DIR}/src ${GENDIR}/src ${CMAKE_CURRENT_BINARY_DIR}/include/)

set(QRC_MAIN_WINDOW src/MainWindow/main_window_resource.qrc)

# Put source code here, files that are generated at build time in
# foedag_generated_SRC
set(foedag_SRC
  src/Tcl/TclInterpreter.cpp
  src/Command/Command.cpp
  src/Command/CommandStack.cpp
  src/MainWindow/main_window.cpp
  src/MainWindow/Session.cpp
  src/Main/qttclnotifier.cpp
  ${QRC_MAIN_WINDOW}
)

add_library(foedag STATIC ${foedag_SRC})

set_target_properties(foedag PROPERTIES PUBLIC_HEADER src/Main/foedag.h)
target_include_directories(foedag PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}/include
  third_party/googletest/googletest/include
  third_party/googletest/googlemock/include)
target_include_directories(foedag PUBLIC $<INSTALL_INTERFACE:include/foedag>)

if(MSVC)
  add_executable(foedag-bin WIN32 ${PROJECT_SOURCE_DIR}/src/Main/main.cpp ${QRC_MAIN_WINDOW})
else()
  add_executable(foedag-bin ${PROJECT_SOURCE_DIR}/src/Main/main.cpp ${QRC_MAIN_WINDOW})
endif()
set_target_properties(foedag-bin PROPERTIES OUTPUT_NAME foedag)

set(TCL_STATIC_LIB libtcl9.0.a)
if (MSVC)
  set(TCL_STATIC_LIB tcl90sx.lib)
endif()

add_custom_target(tcl_build DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/lib/${TCL_STATIC_LIB})
if(MSVC)
  add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/lib/${TCL_STATIC_LIB}
    COMMAND echo "       Compiling Tcl"
    COMMAND nmake -f makefile.vc all install INSTALLDIR=${CMAKE_CURRENT_BINARY_DIR} OPTS=static
    #COMMAND nmake -f makefile.vc all OPTS=static,nomsvcrt,noembed
    COMMAND echo "       Tcl Compilation completed"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/third_party/tcl/win"
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/tcl/generic/tcl.h
  )
else()
  add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/lib/${TCL_STATIC_LIB}
    COMMAND echo "       Compiling Tcl"
    COMMAND ./configure --enable-shared=off --prefix=${CMAKE_CURRENT_BINARY_DIR}
    COMMAND make all -j
    COMMAND make install
    COMMAND echo "       Tcl Compilation completed"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/third_party/tcl/unix"
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/tcl/generic/tcl.h
  )
endif()
# Copy the init.tcl file from source to build directory
add_custom_command(TARGET foedag POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy 
          ${CMAKE_CURRENT_SOURCE_DIR}/third_party/tcl/library/init.tcl
          ${CMAKE_CURRENT_BINARY_DIR}/lib/tcl9.0/init.tcl)
add_dependencies(foedag tcl_build)


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
  # becaues of /IMPLIB option when linking the executable. Unfortunately, there is no documented
  # way to disable the latter in CMake. So, moving the library to the bin directory (right next to the exe)
  set_target_properties(foedag-bin PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
endif()



ADD_LIBRARY(tcl_static STATIC IMPORTED)
SET_TARGET_PROPERTIES(tcl_static  PROPERTIES
    IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/lib/${TCL_STATIC_LIB})

target_link_libraries(foedag-bin PUBLIC foedag tcl_static zlib)
target_link_libraries(foedag  PUBLIC tcl_static zlib)
target_link_libraries(foedag  PUBLIC Qt5::Widgets Qt5::Core Qt5::Gui)

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
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/foedag)
endif()

install(
  FILES ${PROJECT_SOURCE_DIR}/src/MainWindow/main_window.h
        ${PROJECT_SOURCE_DIR}/src/MainWindow/Session.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/foedag/MainWindow)

install(
  FILES ${PROJECT_SOURCE_DIR}/src/Tcl/TclInterpreter.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/foedag/Tcl)

install(
  FILES ${PROJECT_SOURCE_DIR}/src/Command/Command.h
  ${PROJECT_SOURCE_DIR}/src/Command/CommandStack.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/foedag/Command)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/include/tcl.h
        ${CMAKE_CURRENT_BINARY_DIR}/include/tclDecls.h
        ${CMAKE_CURRENT_BINARY_DIR}/include/tclPlatDecls.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/foedag/include)

#install(
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

ADD_CUSTOM_TARGET(link_target ALL
                  COMMAND ${CMAKE_COMMAND} -E create_symlink
                  build/compile_commands.json ../compile_commands.json)
