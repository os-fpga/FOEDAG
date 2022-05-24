#===========================================================================#
#                                                                           #
#    Copyright Notice  -----------------------------------------------------#
#                                                                           #
#    Licensed Materials - Property of QuickLogic Corp.                      #
#    Copyright (C) 2016 QuickLogic Corporation                              #
#    All rights reserved                                                    #
#    Use, duplication, or disclosure restricted                             #
#                                                                           #
#    File   : CMakeLists.txt                                                #
#    Purpose: To compile the source code             	                    #
#    Owner  : Kishor Kumar                                                  #
#                                                                           #
#===========================================================================#
#DIR_PREFIX=cr

#project(crypt LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
else()
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_RELEASE} -Werror")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Werror")
endif()

include (../../cmake/cmake_qt.txt)

set(SOURCE CRFileCryptProc.cpp CRFileCrypt.cpp CRKeyGen.cpp)

set(HEADER_FILES CRFileCryptProc.hpp CRFileCrypt.hpp CRKeyGen.hpp)
set(LIB_NAME cr)

set(CRYPTOPP_LIB_STATIC_LIB libcryptopp.a)



#include_directories("${CMAKE_SOURCE_DIR}/${xml_rel_path}"
#                    "${CMAKE_SOURCE_DIR}/${logging_rel_path}" 
#                    "${CMAKE_SOURCE_DIR}/${misc_rel_path}" 
#                    "${CMAKE_SOURCE_DIR}/${exception_rel_path}"
#     )



include_directories(${PROJECT_SOURCE_DIR}/../../src ${PROJECT_SOURCE_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/../../include/ ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/third_party/cryptopp ${PROJECT_SOURCE_DIR}/third_party/cereal/include)

add_library (${LIB_NAME} STATIC ${SOURCE} ${HEADER_FILES})
target_link_libraries (${LIB_NAME} PUBLIC Qt5::Widgets Qt5::Core Qt5::Gui ${CRYPTOPP_LIB_STATIC_LIB})

add_dependencies(${LIB_NAME} cryptopp_build)


#if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/lib/libcryptopp.a)
#    add_dependencies(${LIB_NAME} log cryptopp)
#else ()
#    add_dependencies(${LIB_NAME} log)
#endif()

#if(WIN32)
#    include_directories(SYSTEM ${BOOST_INCLUDEDIR})
#	target_link_libraries (${LIB_NAME} ${BOOST_LIBS} ${CRYPTOPP_LIB})
#else ()
#    include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
#	target_link_libraries (${LIB_NAME} ${Boost_LIBRARIES} ${CRYPTOPP_LIB})
#endif ()

#add_subdirectory(crTester)

