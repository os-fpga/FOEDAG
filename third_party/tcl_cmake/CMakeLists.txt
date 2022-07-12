# -*- mode:cmake -*-
cmake_minimum_required(VERSION 3.15)

# This cmake encapsulates Tcl, TclStub and Zlib old make builds
# It makes those builds incremental

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

project(tcl_static)

# NOTE: Policy changes has to happen before adding any subprojects
cmake_policy(SET CMP0091 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

#set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_FLAGS "-std=c99")

# NOTE: Set the global output directories after the subprojects have had their go at it
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

if(NOT NO_TCMALLOC)
  find_library(TCMALLOC_LIBRARY NAMES tcmalloc)
  if(TCMALLOC_LIBRARY)
    set(TCMALLOC_COMPILE_OPTIONS
        "-fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free"
    )
  endif()
endif()

set(TCL_STATIC_LIB libtcl8.6.so)
set(TCL_STUBB_LIB libtclstub8.6.a)
set(ZLIB_STATIC_LIB libz.a)
if (MSVC)
  set(TCL_STUBB_LIB tclstub86.lib)
  set(TCL_STATIC_LIB tcl86ts.lib)
  set(ZLIB_STATIC_LIB zlib.lib)
elseif(APPLE) 
  set(TCL_STATIC_LIB libtcl8.6.dylib)
elseif ((DEFINED ENV{MSYSTEM}) AND ("$ENV{MSYSTEM}" STREQUAL "MINGW64"))
  set(TCL_STATIC_LIB libtcl86.dll.a)
  set(ZLIB_STATIC_LIB libzlibstatic.a)
  set(TCL_STUBB_LIB libtclstub86.a)
endif()

if(MSVC)
  add_custom_target(tcl_build DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/../../lib/${TCL_STATIC_LIB})
  add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/../../lib/${TCL_STATIC_LIB}
    COMMAND echo "       Compiling Tcl"
    COMMAND nmake verbose=1 -f makefile.vc INSTALLDIR=${CMAKE_CURRENT_BINARY_DIR}/../../ OPTS=static,nomsvcrt
    COMMAND nmake verbose=1 -f makefile.vc install INSTALLDIR=${CMAKE_CURRENT_BINARY_DIR}/../../ OPTS=static,nomsvcrt
    COMMAND echo "       Tcl Compilation completed"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/../tcl8.6.12/win"
    DEPENDS ../tcl8.6.12/generic/tcl.h
  )
  # Copy the tcl lib from source to build directory
  add_custom_command(TARGET tcl_build POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy 
            ${PROJECT_SOURCE_DIR}/../tcl8.6.12/win/Release_AMD64_VC1932/${TCL_STATIC_LIB}
            ${CMAKE_CURRENT_BINARY_DIR}/../../lib/foedag/lib/${TCL_STATIC_LIB}
  )
  add_custom_target(tcl_stubb_build DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/../../lib/${TCL_STUBB_LIB})
  add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/../../lib/${TCL_STUBB_LIB}
    COMMAND echo "       Compiling Tcl Stubb"
    COMMAND nmake verbose=1 -f makefile.vc INSTALLDIR=${CMAKE_CURRENT_BINARY_DIR}/../../ OPTS=static,nomsvcrt
    COMMAND nmake verbose=1 -f makefile.vc install INSTALLDIR=${CMAKE_CURRENT_BINARY_DIR}/../../ OPTS=static,nomsvcrt
    COMMAND echo "       Tcl Stubb Compilation completed"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/../tcl8.6.12/win"
    DEPENDS ../tcl8.6.12/generic/tcl.h
  )
  # Copy the tcl lib from source to build directory
  add_custom_command(TARGET tcl_stubb_build POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy 
            ${PROJECT_SOURCE_DIR}/../tcl8.6.12/win/Release_AMD64_VC1932/${TCL_STUBB_LIB}
            ${CMAKE_CURRENT_BINARY_DIR}/../../lib/foedag/lib/${TCL_STUBB_LIB}         
  )
else()
  add_custom_target(zlib_build DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/../../lib/${ZLIB_STATIC_LIB})
  add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/../../lib/${ZLIB_STATIC_LIB}
    COMMAND echo "       Compiling Zlib"
    COMMAND ${CMAKE_COMMAND} -P zlib_make.cmake
    COMMAND cp ${ZLIB_STATIC_LIB} ${CMAKE_CURRENT_BINARY_DIR}/../../lib/
    COMMAND echo "       Zlib Compilation completed"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/../zlib"
    DEPENDS ../zlib/zlib.h
  )
  add_custom_target(tcl_build DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/../../lib/${TCL_STATIC_LIB})

  if ((DEFINED ENV{MSYSTEM}) AND ("$ENV{MSYSTEM}" STREQUAL "MINGW64"))
    set(STATIC_LINK --enable-shared=off)
  else((DEFINED ENV{MSYSTEM}) AND ("$ENV{MSYSTEM}" STREQUAL "MINGW64"))
    set(STATIC_LINK )
  endif((DEFINED ENV{MSYSTEM}) AND ("$ENV{MSYSTEM}" STREQUAL "MINGW64"))

  if ((DEFINED ENV{MSYSTEM}) AND ("$ENV{MSYSTEM}" STREQUAL "MINGW64"))
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/../../lib/${TCL_STATIC_LIB}
        COMMAND echo "       Copying TCL Lib"
        COMMAND cp /mingw64/lib/${TCL_STATIC_LIB} ${CMAKE_CURRENT_BINARY_DIR}/../../lib/
        COMMAND echo "       TCL Lib copy completed"
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/../tcl8.6.12/unix"
        DEPENDS ../tcl8.6.12/generic/tcl.h
    )
    add_custom_target(tcl_stubb_build DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/../../lib/${TCL_STUBB_LIB})
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/../../lib/${TCL_STUBB_LIB}
        COMMAND echo "       Copying TCL Stub Lib"
        COMMAND cp /mingw64/lib/${TCL_STUBB_LIB} ${CMAKE_CURRENT_BINARY_DIR}/../../lib/
        COMMAND echo "       TCL Stub Lib copy completed"
        DEPENDS ../tcl8.6.12/generic/tcl.h
    )
  else((DEFINED ENV{MSYSTEM}) AND ("$ENV{MSYSTEM}" STREQUAL "MINGW64"))
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/../../lib/${TCL_STATIC_LIB}
        COMMAND echo "       Compiling Tcl"
        COMMAND rm -rf configure
        COMMAND autoconf configure.in  > configure
        COMMAND chmod 755 configure
        COMMAND ./configure ${STATIC_LINK} --enable-64bit --prefix=${CMAKE_CURRENT_BINARY_DIR}/../..
        COMMAND ${CMAKE_COMMAND} -P ../../tcl_cmake/tcl_make.cmake
        COMMAND echo "       Tcl Compilation completed"
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/../tcl8.6.12/unix"
        DEPENDS ../tcl8.6.12/generic/tcl.h
    )
    add_custom_target(tcl_stubb_build DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/../../lib/${TCL_STUBB_LIB})
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/../../lib/${TCL_STUBB_LIB}
        COMMAND echo "       Compiling Tcl Stubb"
        COMMAND echo "       Tcl Stubb Compilation completed"
        DEPENDS ../tcl8.6.12/generic/tcl.h
    )
  endif((DEFINED ENV{MSYSTEM}) AND ("$ENV{MSYSTEM}" STREQUAL "MINGW64"))
endif()

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

include_directories(${PROJECT_SOURCE_DIR}/src ${GENDIR}/src)

if (WIN32 AND $<CONFIG:Debug>)
  install(
    FILES ${TCL_BINARY_DIR}/runtime/CMakeFiles/tcl_static.dir/tcl_static.pdb
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/tcl_static)
endif()

include(CMakePackageConfigHelpers)

# generate the config file that is includes the exports
configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/TclStaticConfig.cmake"
  INSTALL_DESTINATION cmake
  NO_SET_AND_CHECK_MACRO
  NO_CHECK_REQUIRED_COMPONENTS_MACRO)

# install the configuration file
install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/TclStaticConfig.cmake
  DESTINATION cmake)
