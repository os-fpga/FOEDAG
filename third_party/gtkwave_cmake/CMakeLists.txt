cmake_minimum_required(VERSION 3.15)

# find_program(CCACHE_FOUND ccache)
# if(CCACHE_FOUND)
#     set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
#     set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
# endif(CCACHE_FOUND)

if (${CMAKE_VERSION} VERSION_GREATER "3.8")
    #For cmake >= 3.9 INTERPROCEDURAL_OPTIMIZATION behaviour we need to explicitly
    #set the cmake policy version number
    cmake_policy(VERSION 3.9) 

    # If we are using verison < 3.9 then setting INTERPROCEDURAL_OPTIMIZATION
    # has no effect unless an Intel compiler is used
endif()

# if(NOT CMAKE_BUILD_TYPE)
#     message(WARNING "Build type not set, falling back to Release mode.
#         To specify build type use:
#         -DCMAKE_BUILD_TYPE=<mode> where <mode> is Debug or Release.")
#     set(CMAKE_BUILD_TYPE
#         "Release"
#         CACHE STRING "Choose the type of build, options are: Debug Release."
#         FORCE)
# endif(NOT CMAKE_BUILD_TYPE)

project(GTKWave)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(build_dir ${PROJECT_SOURCE_DIR}/../../dbuild)
else ()
    set(build_dir ${PROJECT_SOURCE_DIR}/../../build)
endif()

set(gtkwave_tar "gtkwave_linux_centos7.tar.gz")
set(download_path "https://github.com/RapidSilicon/post_build_artifacts/releases/download/v0.1/${gtkwave_tar}")
set(temp_path ${CMAKE_CURRENT_SOURCE_DIR}/${gtkwave_tar})
set(install_path ${build_dir}/bin)
set(gtkwave_path ${install_path}/gtkwave)

if(NOT EXISTS ${gtkwave_path})
    if(MSVC)
        message("Windows build not currently supported")
    else()
        message(STATUS "Downloading GTKWave")
        file(DOWNLOAD ${download_path} ${temp_path} SHOW_PROGRESS)
        message(STATUS "Extracting ${gtkwave_tar} to ${install_path}")
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E tar xzf ${temp_path}
            WORKING_DIRECTORY ${install_path}
        )
    endif()
endif()
