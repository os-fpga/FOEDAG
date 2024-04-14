cmake_minimum_required(VERSION 3.15)
set(subsystem cpython)
project(cpython LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/../..)
if(MSVC)
  set(LIB_EXT lib)
  set(LIST_FILE ls)
else()
  set(LIB_EXT a)
  set(LIST_FILE ls)
endif()
if (MSVC)
  set(PYTHON_EXE_NAME "python.exe")
elseif(WIN32)
  set(PYTHON_EXE_NAME "python.exe")
else()
  set(PYTHON_EXE_NAME "python")
endif()

# Directories
set(CPYTHON_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../cpython)
set(TEST_EXE ${subsystem}_test)
# Generate by compiling
set(CPYTHON_LIB libpython3.11.${LIB_EXT})
set(GEN_CPYTHON_LIB ${CPYTHON_SRC_DIR}/${CPYTHON_LIB})
set(GEN_CPYTHON_PY_LIB ${CPYTHON_SRC_DIR}/Lib)
set(GEN_CPYTHON_BIN ${CPYTHON_SRC_DIR}/${PYTHON_EXE_NAME})
set(GEN_CPYTHON_INC ${CPYTHON_SRC_DIR}/Include)
# Build Files
if (MSVC)
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/")
  find_package(CustomPython3 REQUIRED)
  get_filename_component(python3_filename ${Python3_LIBRARIES} NAME)
  set(BUILD_CPYTHON_LIB ${BUILD_DIR}/lib/${python3_filename})
  message(STATUS "MSVC Lib is ${BUILD_CPYTHON_LIB}")
else()
  set(BUILD_CPYTHON_LIB ${BUILD_DIR}/lib/libcpython3.${LIB_EXT})
endif()
set(BUILD_CPYTHON_PY_LIB ${BUILD_DIR}/lib/cpython)
set(BUILD_CPYTHON_BIN ${BUILD_DIR}/${PYTHON_EXE_NAME})
set(BUILD_CPYTHON_INC ${BUILD_DIR}/include/cpython)

if (MINGW OR MSVC OR APPLE)
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/")
  find_package(CustomPython3 REQUIRED)
  message(STATUS "Python3_LIBRARIES (No Build CPython) = ${Python3_LIBRARIES}")
  message(STATUS "Python3_EXECUTABLE (No Build CPython) = ${Python3_EXECUTABLE}")
  message(STATUS "Python3_INCLUDE_DIRS (No Build CPython) = ${Python3_INCLUDE_DIRS}")
  message(STATUS "Python3_LIBRARY_DIRS (No Build CPython) = ${Python3_LIBRARY_DIRS}")
  set(GEN_CPYTHON_LIB ${Python3_LIBRARIES})
  set(GEN_CPYTHON_PY_LIB ${Python3_LIBRARY_DIRS})
  set(GEN_CPYTHON_BIN ${Python3_EXECUTABLE})
  set(GEN_CPYTHON_INC ${Python3_INCLUDE_DIRS})
  add_custom_target(
    cpython_gen
    COMMAND echo "*** Start of No Build CPython - just copy ***"
    COMMAND echo "*** End of CPython custom build - list ***"
  )
else()
  set(Python3_INCLUDE_DIRS ${CPYTHON_SRC_DIR}/Include)
  if (EXISTS ${GEN_CPYTHON_LIB} AND EXISTS ${GEN_CPYTHON_BIN})
    add_custom_target(
      cpython_gen
      COMMAND echo "*** Start of CPython custom build - just copy ***"
      COMMAND ${LIST_FILE} ${GEN_CPYTHON_LIB}
      COMMAND ${LIST_FILE} ${GEN_CPYTHON_BIN}
      COMMAND echo "*** End of CPython custom build - list ***"
    )
  elseif (MSVC)
    # Not being called, but prepare for future
    add_custom_target(
        cpython_gen
        COMMAND echo "*** Start of CPython custom build (MSVC) ***"
        COMMAND cd ${CPYTHON_SRC_DIR} && perl ./configure VC-WIN64A && nmake
        COMMAND ${LIST_FILE} ${GEN_CPYTHON_LIB}
        COMMAND ${LIST_FILE} ${GEN_CPYTHON_BIN}
        COMMAND echo "*** End of CPython custom build - list ***"
      )
  else()
    add_custom_target(
      cpython_gen
      COMMAND echo "*** Start of CPython custom build ***"
      COMMAND cd ${CPYTHON_SRC_DIR} && sh ./configure && make
      COMMAND ${LIST_FILE} ${GEN_CPYTHON_LIB}
      COMMAND ${LIST_FILE} ${GEN_CPYTHON_BIN}
      COMMAND echo "*** End of CPython custom build - list ***"
    )
  endif()
endif()
if (MSVC)
  set(ADDITIONAL_LIB "")
elseif(WIN32)
  set(ADDITIONAL_LIB "ws2_32")
else()
  list(APPEND ADDITIONAL_LIB dl pthread util)
endif()
message("CMake System Name: ${CMAKE_SYSTEM_NAME}")
message("Additional Lib: ${ADDITIONAL_LIB}")

add_custom_command(
  OUTPUT ${BUILD_CPYTHON_LIB}
  DEPENDS cpython_gen
  COMMAND ${CMAKE_COMMAND} -E copy ${GEN_CPYTHON_LIB} ${BUILD_CPYTHON_LIB}
)

add_custom_command(
  OUTPUT ${BUILD_CPYTHON_PY_LIB}
  DEPENDS cpython_gen
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${GEN_CPYTHON_PY_LIB} ${BUILD_CPYTHON_PY_LIB}
)

add_custom_command(
  OUTPUT ${BUILD_CPYTHON_BIN}
  DEPENDS cpython_gen
  COMMAND ${CMAKE_COMMAND} -E copy ${GEN_CPYTHON_BIN} ${BUILD_CPYTHON_BIN}
)

if (MINGW OR MSVC OR APPLE)
  add_custom_command(
    OUTPUT ${BUILD_CPYTHON_INC}
    DEPENDS cpython_gen
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${GEN_CPYTHON_INC} ${BUILD_CPYTHON_INC}
  )
else()
  add_custom_command(
    OUTPUT ${BUILD_CPYTHON_INC}
    DEPENDS cpython_gen
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${GEN_CPYTHON_INC} ${BUILD_CPYTHON_INC}
    COMMAND ${CMAKE_COMMAND} -E copy ${CPYTHON_SRC_DIR}/pyconfig.h ${BUILD_CPYTHON_INC}/.
  )
endif()

add_custom_target(
  cpython_build ALL
  DEPENDS cpython_gen ${BUILD_CPYTHON_LIB} ${BUILD_CPYTHON_PY_LIB} ${BUILD_CPYTHON_BIN} ${BUILD_CPYTHON_INC}
)

add_executable(
  ${TEST_EXE}
  cpython_test.cpp
)
add_dependencies(${TEST_EXE} cpython_build)
target_link_libraries(${TEST_EXE} ${BUILD_CPYTHON_LIB} ${ADDITIONAL_LIB})
target_include_directories(${TEST_EXE} BEFORE PRIVATE ${BUILD_CPYTHON_INC})

add_custom_target(
  cpython_lib_test ALL
  DEPENDS cpython_build ${TEST_EXE}
  COMMAND ${TEST_EXE} "${BUILD_DIR}"
)
