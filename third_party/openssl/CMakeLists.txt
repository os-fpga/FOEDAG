cmake_minimum_required(VERSION 3.15)
set(subsystem openssl)
project(openssl LANGUAGES CXX)

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

set(CRYPTO_LIB libcrypto.${LIB_EXT})
set(SSL_LIB libssl.${LIB_EXT})
set(GEN_CRYPTO ${CMAKE_CURRENT_SOURCE_DIR}/${CRYPTO_LIB})
set(GEN_SSL ${CMAKE_CURRENT_SOURCE_DIR}/${SSL_LIB})
set(SOURCE_OPENSSL_INC ${CMAKE_CURRENT_SOURCE_DIR}/include/openssl)
set(BUILD_CRYPTO ${BUILD_DIR}/lib/libopenssl_crypto_1.1.1t.${LIB_EXT})
set(BUILD_SSL ${BUILD_DIR}/lib/libopenssl_ssl_1.1.1t.${LIB_EXT})
set(BUILD_OPENSSL_INC ${BUILD_DIR}/include/openssl)
set(TEST_EXE ${subsystem}_test)

if(EXISTS ${GEN_CRYPTO} AND EXISTS ${GEN_SSL})
  add_custom_target(
    openssl_gen
    COMMAND echo "*** Start of OpenSSL custom build - just copy ***"
    COMMAND ${LIST_FILE} ${CMAKE_CURRENT_SOURCE_DIR}/*.${LIB_EXT}
    COMMAND echo "*** End of OpenSSL custom build - list ***"
  )
elseif(MSVC)
  add_custom_target(
    openssl_gen
    COMMAND echo "*** Start of OpenSSL custom build ***"
    COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR} && perl Configure VC-WIN64A && nmake
    COMMAND ${LIST_FILE} ${CMAKE_CURRENT_SOURCE_DIR}/*.${LIB_EXT}
    COMMAND echo "*** End of OpenSSL custom build - list ***"
  )
else()
  add_custom_target(
    openssl_gen
    COMMAND echo "*** Start of OpenSSL custom build ***"
    COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR} && sh config && make
    COMMAND ${LIST_FILE} ${CMAKE_CURRENT_SOURCE_DIR}/*.${LIB_EXT}
    COMMAND echo "*** End of OpenSSL custom build - list ***"
  )
endif()
if (MSVC)
  set(ADDITIONAL_LIB "")
else()
  list(APPEND ADDITIONAL_LIB dl pthread)
endif()

add_custom_command(
  OUTPUT ${BUILD_CRYPTO}
  DEPENDS openssl_gen
  COMMAND ${CMAKE_COMMAND} -E copy ${GEN_CRYPTO} ${BUILD_CRYPTO}
)

add_custom_command(
  OUTPUT ${BUILD_SSL}
  DEPENDS openssl_gen
  COMMAND ${CMAKE_COMMAND} -E copy ${GEN_SSL} ${BUILD_SSL}
)

add_custom_command(
  OUTPUT ${BUILD_OPENSSL_INC}
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${SOURCE_OPENSSL_INC} ${BUILD_OPENSSL_INC}
  COMMAND ${LIST_FILE} -la ${BUILD_OPENSSL_INC}/opensslconf.h
)

add_custom_target(
  openssl_build ALL
  DEPENDS openssl_gen ${BUILD_CRYPTO} ${BUILD_SSL} ${BUILD_OPENSSL_INC}
)

add_executable(
  ${TEST_EXE}
  openssl_test.cpp
)
add_dependencies(${TEST_EXE} openssl_build)
target_link_libraries(${TEST_EXE} ${BUILD_CRYPTO} ${BUILD_SSL} ${ADDITIONAL_LIB})
link_directories(${BUILD_DIR}/include)

add_custom_target(
  openssl_lib_test ALL
  DEPENDS openssl_build ${TEST_EXE}
  COMMAND ${TEST_EXE}
)
