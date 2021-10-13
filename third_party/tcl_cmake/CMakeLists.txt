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


add_custom_command(
  OUTPUT ../../build/third_party/tcl_cmake/lib/libtcl9.0.a
  COMMAND echo "       Compiling Tcl"
  COMMAND ./configure --enable-shared=off --prefix=${PWD}/../../../build
  COMMAND echo "       Tcl Compilation completed"
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tcl/unix"
)




ADD_LIBRARY(tcl_static STATIC IMPORTED)
SET_TARGET_PROPERTIES(tcl_static  PROPERTIES
    IMPORTED_LOCATION ../../build/third_party/tcl_cmake/lib/libtcl9.0.a)

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


#add_library(tcl_static STATIC ${tcl_static_SRC})
#set_target_properties(tcl_static PROPERTIES PUBLIC_HEADER src/tcl.h)

# Installation target
#install(
#  TARGETS tcl_static
#  EXPORT TclStatic
#  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/tcl
#  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/tcl)

if (WIN32 AND $<CONFIG:Debug>)
  install(
    FILES ${TCL_BINARY_DIR}/runtime/CMakeFiles/tcl_static.dir/tcl_static.pdb
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/tcl_static)
endif()

#install(
#  EXPORT TclStatic
#  FILE TclStatic.cmake
#  DESTINATION cmake)
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
