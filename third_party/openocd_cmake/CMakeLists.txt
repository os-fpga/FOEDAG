cmake_minimum_required(VERSION 3.15)

if (${CMAKE_VERSION} VERSION_GREATER "3.8")
    #For cmake >= 3.9 INTERPROCEDURAL_OPTIMIZATION behaviour we need to explicitly
    #set the cmake policy version number
    cmake_policy(VERSION 3.9) 

    # If we are using verison < 3.9 then setting INTERPROCEDURAL_OPTIMIZATION
    # has no effect unless an Intel compiler is used
endif()

if(NOT CMAKE_BUILD_TYPE)
    message(WARNING "Build type not set, falling back to Release mode.
        To specify build type use:
        -DCMAKE_BUILD_TYPE=<mode> where <mode> is Debug or Release.")
    set(CMAKE_BUILD_TYPE
        "Release"
        CACHE STRING "Choose the type of build, options are: Debug Release."
        FORCE)
endif(NOT CMAKE_BUILD_TYPE)

project(rs_openocd)

if(RAPTOR)
    set(root_dir ${PROJECT_SOURCE_DIR}/../../../..)
else()
    if ((EXISTS ${PROJECT_SOURCE_DIR}/../../build) OR (EXISTS ${PROJECT_SOURCE_DIR}/../../dbuild))
        set(root_dir ${PROJECT_SOURCE_DIR}/../..)
    else()
        set(root_dir ${PROJECT_SOURCE_DIR}/../../..)
    endif()
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(build_dir ${root_dir}/dbuild)
else ()
    set(build_dir ${root_dir}/build)
endif()

if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    # <TODO>
    #set(rs_openocd_tar "rs_openocd-win.zip")
else()
    set(rs_openocd_tar "openocd-7f5a821-linux86_64.tar.gz1")
endif()

set(download_path "https://github.com/RapidSilicon/openocd/releases/download/latest/${rs_openocd_tar}")
if(WIN32)
    # <TODO> - set temp_path to rs_openocd zip file when windows build available
    # set(temp_path "${CMAKE_CURRENT_SOURCE_DIR}/rs_openocd-win.zip")
else()
    set(temp_path ${CMAKE_CURRENT_SOURCE_DIR}/openocd-7f5a821-linux86_64.tar.gz)
endif()
set(install_path ${build_dir}/bin)
set(rs_openocd_path ${install_path}/openocd)

if(NOT EXISTS ${temp_path})
    message(STATUS "Downloading openocd from ${download_path}")
    file(DOWNLOAD ${download_path} ${temp_path})
endif()

add_library(rs_openocd)

message(STATUS "Creating directory install path: ${install_path}")
execute_process(
    COMMAND ${CMAKE_COMMAND} -E make_directory ${install_path}
)

set(EXTRACTED_FILE_TEMP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/temp_dir)
file(MAKE_DIRECTORY ${EXTRACTED_FILE_TEMP_DIR})

set(files_to_extract 
    "opt/openocd/bin/openocd" 
    "opt/openocd/share/openocd/scripts/board/gemini.cfg")

message(STATUS "Extracting ${rs_openocd_tar} to ${install_path}")
execute_process(
    COMMAND ${CMAKE_COMMAND} -E tar xzf ${temp_path} ${files_to_extract}
    WORKING_DIRECTORY ${EXTRACTED_FILE_TEMP_DIR}
)

execute_process(
    COMMAND ${CMAKE_COMMAND} -E make_directory ${install_path}/openocd
)

execute_process(
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${EXTRACTED_FILE_TEMP_DIR}/opt/openocd
        ${install_path}/openocd
)

if(EXISTS ${EXTRACTED_FILE_TEMP_DIR})
    file(REMOVE_RECURSE ${EXTRACTED_FILE_TEMP_DIR})
endif()
