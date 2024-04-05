cmake_minimum_required(VERSION 3.15)
set(subsystem cfgpython)
project(cfgpython LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/../../..)
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

set(TEST_EXE ${subsystem}_test)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../../../cmake/")
find_package(CustomPython3 REQUIRED)
get_filename_component(python3_lib_filename ${Python3_LIBRARIES} NAME)
message(STATUS "Python3_LIBRARIES (CFGPython) = ${Python3_LIBRARIES}")
message(STATUS "Python3_EXECUTABLE (CFGPython) = ${Python3_EXECUTABLE}")
message(STATUS "Python3_INCLUDE_DIRS (CFGPython) = ${Python3_INCLUDE_DIRS}")
message(STATUS "Python3_LIBRARY_DIRS (CFGPython) = ${Python3_LIBRARY_DIRS}")
message(STATUS "Python3_PY_LIBRARY_DIR (CFGPython) = ${Python3_PY_LIBRARY_DIR}")
set(GEN_CPYTHON_LIB ${Python3_LIBRARIES})
set(GEN_CPYTHON_PY_LIB ${Python3_PY_LIBRARY_DIR})
set(GEN_CPYTHON_BIN ${Python3_EXECUTABLE})
set(GEN_CPYTHON_INC ${Python3_INCLUDE_DIRS})
# Build Files
set(BUILD_CPYTHON_LIB ${BUILD_DIR}/lib/cfgpython/${python3_lib_filename})
set(BUILD_CPYTHON_PY_LIB ${BUILD_DIR}/lib/cfgpython_py)
set(BUILD_CPYTHON_BIN ${BUILD_DIR}/bin/${PYTHON_EXE_NAME})
set(BUILD_CPYTHON_INC ${BUILD_DIR}/include/cfgpython)
if (MSVC)
  set(ADDITIONAL_LIB "")
elseif(WIN32)
  set(ADDITIONAL_LIB "ws2_32")
else()
  list(APPEND ADDITIONAL_LIB dl pthread z expat)
endif()
message("CMake System Name: ${CMAKE_SYSTEM_NAME}")
message("Additional Lib: ${ADDITIONAL_LIB}")

add_custom_command(
  OUTPUT ${BUILD_CPYTHON_LIB}
  COMMAND ${CMAKE_COMMAND} -E copy ${GEN_CPYTHON_LIB} ${BUILD_CPYTHON_LIB}
)

add_custom_command(
  OUTPUT ${BUILD_CPYTHON_PY_LIB}
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${GEN_CPYTHON_PY_LIB} ${BUILD_CPYTHON_PY_LIB}
)

add_custom_command(
  OUTPUT ${BUILD_CPYTHON_BIN}
  COMMAND ${CMAKE_COMMAND} -E copy ${GEN_CPYTHON_BIN} ${BUILD_CPYTHON_BIN}
)

add_custom_command(
  OUTPUT ${BUILD_CPYTHON_INC}
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${GEN_CPYTHON_INC} ${BUILD_CPYTHON_INC}
)

add_custom_target(
  cfgpython ALL
  DEPENDS ${BUILD_CPYTHON_LIB} ${BUILD_CPYTHON_PY_LIB} ${BUILD_CPYTHON_BIN} ${BUILD_CPYTHON_INC}
)

add_executable(
  ${TEST_EXE}
  cpython_test.cpp
)
add_dependencies(${TEST_EXE} cfgpython)
target_link_libraries(${TEST_EXE} ${BUILD_CPYTHON_LIB} ${ADDITIONAL_LIB})
target_include_directories(${TEST_EXE} BEFORE PRIVATE ${BUILD_CPYTHON_INC})

add_custom_target(
  cpython_lib_test ALL
  DEPENDS cfgpython ${TEST_EXE}
  COMMAND ${TEST_EXE} "${BUILD_DIR}"
)
