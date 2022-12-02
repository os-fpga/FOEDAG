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

project(GTKWave)

if(RAPTOR)
    set(proj_dir raptor)
    set(root_dir ${PROJECT_SOURCE_DIR}/../../../..)
else()
    set(proj_dir foedag)
    set(root_dir ${PROJECT_SOURCE_DIR}/../..)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(build_dir ${root_dir}/dbuild)
else ()
    set(build_dir ${root_dir}/build)
endif()

if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    set(gtkwave_tar "gtkwave-3.3.100-bin-win32.zip")
else()
    set(gtkwave_tar "gtkwaveTCL.tar.gz")
endif()

set(download_path "https://github.com/RapidSilicon/post_build_artifacts/releases/download/v0.1/${gtkwave_tar}")
set(temp_path ${CMAKE_CURRENT_SOURCE_DIR}/${gtkwave_tar})
set(install_path ${build_dir}/share/${proj_dir})
set(gtkwave_path ${install_path}/gtkwave)

if(skip)
    # remove this once sourceforge slow downloads are resolved
    message("GTKWave: Windows not currently supported")
else()
    if(NOT EXISTS ${gtkwave_path})
        file(MAKE_DIRECTORY ${install_path})
        message(STATUS "Downloading GTKWave from ${download_path}")
        file(DOWNLOAD ${download_path} ${temp_path} SHOW_PROGRESS)
        message(STATUS "Extracting ${gtkwave_tar} to ${install_path}")
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E tar xzf ${temp_path}
            WORKING_DIRECTORY ${install_path}
        )
    endif()
endif()

install(
    DIRECTORY ${install_path}/gtkwave DESTINATION  ${CMAKE_INSTALL_DATAROOTDIR}/${proj_dir}
    USE_SOURCE_PERMISSIONS
)
