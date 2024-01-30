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
set(OPENSSL_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../openssl)
set(GEN_CRYPTO ${OPENSSL_SRC_DIR}/${CRYPTO_LIB})
set(GEN_SSL ${OPENSSL_SRC_DIR}/${SSL_LIB})
set(SOURCE_OPENSSL_INC ${OPENSSL_SRC_DIR}/include/openssl)
set(BUILD_CRYPTO ${BUILD_DIR}/lib/libopenssl_crypto_3.0.9.${LIB_EXT})
set(BUILD_SSL ${BUILD_DIR}/lib/libopenssl_ssl_3.0.9.${LIB_EXT})
set(BUILD_OPENSSL_INC ${BUILD_DIR}/include/openssl)
set(TEST_EXE ${subsystem}_test)

if(EXISTS ${GEN_CRYPTO} AND EXISTS ${GEN_SSL})
  add_custom_target(
    openssl_gen
    COMMAND echo "*** Start of OpenSSL custom build - just copy ***"
    COMMAND ${LIST_FILE} ${OPENSSL_SRC_DIR}/*.${LIB_EXT}
    COMMAND echo "*** End of OpenSSL custom build - list ***"
  )
elseif(MSVC)
  add_custom_target(
    openssl_gen
    COMMAND echo "*** Start of OpenSSL custom build (MSVC) ***"
    COMMAND cd ${OPENSSL_SRC_DIR} && perl Configure VC-WIN64A && nmake
    COMMAND ${LIST_FILE} ${OPENSSL_SRC_DIR}/*.${LIB_EXT}
    COMMAND echo "*** End of OpenSSL custom build - list ***"
  )
else()
  add_custom_target(
    openssl_gen
    COMMAND echo "*** Start of OpenSSL custom build ***"
    COMMAND cd ${OPENSSL_SRC_DIR} && sh config && make
    COMMAND ${LIST_FILE} ${OPENSSL_SRC_DIR}/*.${LIB_EXT}
    COMMAND echo "*** End of OpenSSL custom build - list ***"
  )
endif()
if (MSVC)
  set(ADDITIONAL_LIB "")
elseif(WIN32)
  set(ADDITIONAL_LIB "ws2_32")
else()
  list(APPEND ADDITIONAL_LIB dl pthread)
endif()
message("CMake System Name: ${CMAKE_SYSTEM_NAME}")
message("Additional Lib: ${ADDITIONAL_LIB}")

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
target_include_directories(${TEST_EXE} BEFORE PRIVATE ${BUILD_DIR}/include)

add_custom_target(
  openssl_lib_test ALL
  DEPENDS openssl_build ${TEST_EXE}
  COMMAND ${TEST_EXE}
)

if(UNIX)
add_custom_command(TARGET openssl_build POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${OPENSSL_SRC_DIR}/libcrypto.so.3 ${CMAKE_BINARY_DIR}/lib
    COMMAND ${CMAKE_COMMAND} -E create_symlink libcrypto.so.3 libcrypto.so	
    COMMAND ${CMAKE_COMMAND} -E copy ${OPENSSL_SRC_DIR}/libssl.so.3 ${CMAKE_BINARY_DIR}/lib
    COMMAND ${CMAKE_COMMAND} -E create_symlink libssl.so.3  libssl.so
    COMMENT "Copying lib ssl and crypto after building the OpenSSL"
           WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/lib

)

install(
    PROGRAMS
        ${CMAKE_BINARY_DIR}/lib/libcrypto.so.3
	${CMAKE_BINARY_DIR}/lib/libssl.so.3
    DESTINATION
        ${CMAKE_INSTALL_LIBDIR}
)


install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink libssl.so.3  libssl.so WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR})")

install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink libcrypto.so.3 libcrypto.so WORKING_DIRECTORY  ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR})")
endif()






