cmake_minimum_required(VERSION 3.15)

project(scope_guard LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${PROJECT_SOURCE_DIR}/../../src ${PROJECT_SOURCE_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/../../include/)

set (SRC_H_LIST
    scope_guard.hpp
)

add_library(scope_guard STATIC
  ${SRC_H_LIST}
)

target_link_libraries(scope_guard PUBLIC)
add_custom_command(
  TARGET scope_guard POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy
          ${PROJECT_SOURCE_DIR}/scope_guard.hpp
          ${CMAKE_CURRENT_BINARY_DIR}/../../include/scope_guard/scope_guard.hpp
)

install(
   FILES ${PROJECT_SOURCE_DIR}/scope_guard.hpp
   DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/foedag/scope_guard)

