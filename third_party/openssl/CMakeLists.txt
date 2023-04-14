cmake_minimum_required(VERSION 3.15)

project(openssl LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/../..)
if(MSVC)
  set(CRYPTO_LIB libcrypto-1_1-x64.dll)
  set(SSL_LIB libssl-1_1-x64.dll)
  set(LIB_EXT dll)
  set(LIST_FILE ls)
else()
  set(CRYPTO_LIB libcrypto.a)
  set(SSL_LIB libssl.a)
  set(LIB_EXT a)
  set(LIST_FILE ls)
endif()

if(MSVC)
add_custom_target(
  openssl_build ALL
  COMMAND echo "*** Start of OpenSSL custom build - just copy ***"
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/${CRYPTO_LIB} ${BUILD_DIR}/lib/libopenssl_crypto_1.1.1t.${LIB_EXT}
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/${SSL_LIB} ${BUILD_DIR}/lib/libopenssl_ssl_1.1.1t.${LIB_EXT}
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/include/openssl ${BUILD_DIR}/include/openssl
  COMMAND echo "*** End of OpenSSL custom build ***"
  COMMAND ${LIST_FILE} ${CMAKE_CURRENT_SOURCE_DIR}/*.${LIB_EXT}
  COMMAND echo "*** End of OpenSSL custom build - list ***"
)
else()
add_custom_target(
  openssl_build ALL
  COMMAND echo "*** Start of OpenSSL custom build ***"
  COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR} && sh config && make
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/${CRYPTO_LIB} ${BUILD_DIR}/lib/libopenssl_crypto_1.1.1t.${LIB_EXT}
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/${SSL_LIB} ${BUILD_DIR}/lib/libopenssl_ssl_1.1.1t.${LIB_EXT}
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/include/openssl ${BUILD_DIR}/include/openssl
  COMMAND echo "*** End of OpenSSL custom build ***"
  COMMAND ${LIST_FILE} ${CMAKE_CURRENT_SOURCE_DIR}/*.${LIB_EXT}
  COMMAND echo "*** End of OpenSSL custom build - list ***"
)
endif()

