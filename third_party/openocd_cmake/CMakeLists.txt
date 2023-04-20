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
    set(rs_openocd_tar "openocd-i686-w64-mingw32.tar.gz")
else()
    set(rs_openocd_tar "openocd-linux86_64.tar.gz")
endif()

if(RAPTOR)
    set(Destination_Dir ${build_dir}/share/raptor/configuration)
else()
    if ((EXISTS ${PROJECT_SOURCE_DIR}/../../build) OR (EXISTS ${PROJECT_SOURCE_DIR}/../../dbuild))
        set(Destination_Dir ${build_dir}/share/foedag/configuration)
    else()
        set(Destination_Dir ${build_dir}/share/raptor/configuration)
    endif()
endif()

set(download_path "https://github.com/RapidSilicon/openocd/releases/download/latest/${rs_openocd_tar}")
if(WIN32)
    set(temp_path ${CMAKE_CURRENT_SOURCE_DIR}/openocd-i686-w64-mingw32.tar.gz)
    set(files_to_extract 
        "bin/openocd.exe" 
        "share/openocd/scripts/board/gemini.cfg")
else()
    set(temp_path ${CMAKE_CURRENT_SOURCE_DIR}/openocd-linux86_64.tar.gz)
    set(files_to_extract 
        "opt/openocd/bin/openocd" 
        "opt/openocd/share/openocd/scripts/board/gemini.cfg")
endif()

set(bin_path ${build_dir}/bin)

if(NOT EXISTS ${temp_path})
    file(DOWNLOAD ${download_path} ${temp_path})
endif()

execute_process(
    COMMAND ${CMAKE_COMMAND} -E make_directory ${bin_path}
)

set(EXTRACTED_FILE_TEMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/temp_dir)
file(MAKE_DIRECTORY ${EXTRACTED_FILE_TEMP_DIR})

if(WIN32)
    add_custom_target(rs_openocd_extract_config_and_openocd ALL
        COMMENT "Extract openocd to ${bin_path} and move gemini.cfg to ${Destination_Dir}"
        DEPENDS ${temp_path}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${Destination_Dir}
        COMMAND ${CMAKE_COMMAND} -E tar xzf ${temp_path} ${files_to_extract}
        WORKING_DIRECTORY ${EXTRACTED_FILE_TEMP_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy ${EXTRACTED_FILE_TEMP_DIR}/share/openocd/scripts/board/gemini.cfg ${Destination_Dir}
        COMMAND ${CMAKE_COMMAND} -E copy ${EXTRACTED_FILE_TEMP_DIR}/bin/openocd.exe ${bin_path}
    )
else()
    add_custom_target(rs_openocd_extract_config_and_openocd ALL
        COMMENT "Extract openocd to ${bin_path} and move gemini.cfg to ${Destination_Dir}"
        DEPENDS ${temp_path}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${Destination_Dir}
        COMMAND ${CMAKE_COMMAND} -E tar xzf ${temp_path} ${files_to_extract}
        WORKING_DIRECTORY ${EXTRACTED_FILE_TEMP_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy ${EXTRACTED_FILE_TEMP_DIR}/opt/openocd/share/openocd/scripts/board/gemini.cfg ${Destination_Dir}
        COMMAND ${CMAKE_COMMAND} -E copy ${EXTRACTED_FILE_TEMP_DIR}/opt/openocd/bin/openocd ${bin_path}
    )
endif()

