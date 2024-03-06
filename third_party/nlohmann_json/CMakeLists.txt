cmake_minimum_required(VERSION 3.15)

project(nlohmann_json LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${PROJECT_SOURCE_DIR}/../../src ${PROJECT_SOURCE_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/../../include/)

set (SRC_H_LIST
    json.hpp
)

add_custom_target(foedag_nlohmann_json DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/../../include/nlohmann_json/json.hpp)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/../../include/nlohmann_json/json.hpp
  COMMAND echo "       Copying ${CMAKE_CURRENT_BINARY_DIR}/../../include/nlohmann_json/json.hpp"
  COMMAND ${CMAKE_COMMAND} -E copy
          ${PROJECT_SOURCE_DIR}/json.hpp
          ${CMAKE_CURRENT_BINARY_DIR}/../../include/nlohmann_json/json.hpp
  DEPENDS ${PROJECT_SOURCE_DIR}/json.hpp
)

install(
   FILES ${PROJECT_SOURCE_DIR}/json.hpp
   DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/foedag/nlohmann_json)

