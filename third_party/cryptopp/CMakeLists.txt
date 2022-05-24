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

project(cryptopp)

include(ProcessorCount)
ProcessorCount(N)
set(MAKEFLAGS '')
if(NOT N EQUAL 0)
  set(MAKEFLAGS -j${N})
endif()

set(CRYPTOPP_LIB_STATIC_LIB libcryptopp.a)

add_custom_target(cryptopp_build DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/../../lib/${CRYPTOPP_LIB_STATIC_LIB})
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/../../lib/${CRYPTOPP_LIB_STATIC_LIB}
    COMMAND echo "       Compiling cryptopp"
    COMMAND $(MAKE) clean
    COMMAND $(MAKE) ${MAKEFLAGS}
    COMMAND $(MAKE) install PREFIX=${CMAKE_CURRENT_BINARY_DIR}/../../
    #COMMAND ${CMAKE_COMMAND} -P cryptopp_make.cmake
    #COMMAND echo "       copying built .a file"
    #COMMAND cp ${CRYPTOPP_LIB_STATIC_LIB} ${CMAKE_CURRENT_BINARY_DIR}/../../lib/
    COMMAND echo "       cryptopp Compilation completed"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/../cryptopp"
    )
