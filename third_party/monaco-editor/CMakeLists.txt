cmake_minimum_required(VERSION 3.15)

if (USE_MONACO_EDITOR)

set(MONACO_EDITOR_VERSION 0.45.0)

# download the monaco editor tgz, extract it, and rename the dir from 'package' to 'monaco-editor'
if(NOT EXISTS ${CMAKE_SOURCE_DIR}/third_party/monaco-editor/monaco-editor-${MONACO_EDITOR_VERSION}.tgz)
message("Downloading monaco-editor tgz package: " ${MONACO_EDITOR_VERSION} " ...")
file(DOWNLOAD
    https://registry.npmjs.org/monaco-editor/-/monaco-editor-${MONACO_EDITOR_VERSION}.tgz
    ${CMAKE_SOURCE_DIR}/third_party/monaco-editor/monaco-editor-${MONACO_EDITOR_VERSION}.tgz
)
endif()

if(NOT EXISTS ${CMAKE_SOURCE_DIR}/third_party/monaco-editor/monaco-editor/)
message("Extracting monaco-editor tgz package: " ${MONACO_EDITOR_VERSION} " ...")
execute_process(
    COMMAND_ECHO
        STDOUT
    COMMAND 
        tar -xf ${CMAKE_SOURCE_DIR}/third_party/monaco-editor/monaco-editor-${MONACO_EDITOR_VERSION}.tgz
    RESULT_VARIABLE result
    WORKING_DIRECTORY
        ${CMAKE_SOURCE_DIR}/third_party/monaco-editor
)
if(result AND NOT result EQUAL 0)
    message(FATAL_ERROR "monaco editor extraction failed!")
endif()
file(RENAME ${CMAKE_SOURCE_DIR}/third_party/monaco-editor/package ${CMAKE_SOURCE_DIR}/third_party/monaco-editor/monaco-editor)
endif()

endif(USE_MONACO_EDITOR)
