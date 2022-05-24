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
#DIR_PREFIX=crt
set (SOURCE CRTestMain.cpp)
set (TESTER_EXE crTester)

include_directories("${CMAKE_SOURCE_DIR}/${exception_rel_path}"
                    "${CMAKE_SOURCE_DIR}/${logging_rel_path}" 
                    "${CMAKE_SOURCE_DIR}/${misc_rel_path}"
                    "${CMAKE_SOURCE_DIR}/${crypt_rel_path}"
)

set (LIB_DEPEND cr misc ex log)
add_executable (${TESTER_EXE} ${SOURCE})

######### Not to give linker warning "*.pdb not found" #########################
if(WIN32)
#set_target_properties(crTester PROPERTIES LINK_FLAGS "/ignore:4099")
endif()

if(WIN32)
    include_directories(SYSTEM ${BOOST_INCLUDEDIR} ${CRYPTOPP_INC_DIR})
    target_link_libraries (${TESTER_EXE} ${LIB_DEPEND} ${BOOST_LIBS} ${CRYPTOPP_LIB} ${ZLIB_LIB})
elseif (APPLE)
    target_link_directories(${TESTER_EXE} PRIVATE ${ZSTD_LIBDIR})
    include_directories(SYSTEM ${Boost_INCLUDE_DIRS} ${QT_INCLUDE_DIRS} ${CRYPTOPP_INC_DIR})
    target_link_libraries (${TESTER_EXE} ${LIB_DEPEND} edit dl z ${Boost_LIBRARIES} ${CRYPTOPP_LIB})
else()
    include_directories(SYSTEM ${Boost_INCLUDE_DIRS} ${CRYPTOPP_INC_DIR})
    target_link_libraries (${TESTER_EXE} ${LIB_DEPEND} pthread dl z ${Boost_LIBRARIES} ${CRYPTOPP_LIB})
endif()
			
install(
    TARGETS ${TESTER_EXE}
    RUNTIME DESTINATION ${BIN_INSTALL_DIR}
    LIBRARY DESTINATION ${LIB_INSTALL_DIR}
    ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
    )

