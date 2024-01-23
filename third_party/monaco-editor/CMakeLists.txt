cmake_minimum_required(VERSION 3.15)

set(MONACO_EDITOR_VERSION 0.45.0)

# download the monaco editor tgz, extract it, and rename the dir from 'package' to 'monaco-editor'
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/monaco-editor-${MONACO_EDITOR_VERSION}.tgz)
message("Downloading monaco-editor tgz package: " ${MONACO_EDITOR_VERSION} " ...")
file(DOWNLOAD
    https://registry.npmjs.org/monaco-editor/-/monaco-editor-${MONACO_EDITOR_VERSION}.tgz
    ${CMAKE_CURRENT_SOURCE_DIR}/monaco-editor-${MONACO_EDITOR_VERSION}.tgz
)
endif()

if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/monaco-editor/)
message("Extracting monaco-editor tgz package: " ${MONACO_EDITOR_VERSION} " ...")
execute_process(
    COMMAND_ECHO
        STDOUT
    COMMAND 
        tar -xf ${CMAKE_CURRENT_SOURCE_DIR}/monaco-editor-${MONACO_EDITOR_VERSION}.tgz
    RESULT_VARIABLE result
    WORKING_DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}
)
if(result AND NOT result EQUAL 0)
    message(FATAL_ERROR "monaco editor extraction failed!")
endif()
file(RENAME ${CMAKE_CURRENT_SOURCE_DIR}/package ${CMAKE_CURRENT_SOURCE_DIR}/monaco-editor)
endif()

execute_process(
    COMMAND ${CMAKE_COMMAND} -E make_directory  ${CMAKE_SOURCE_DIR}/etc/monaco-editor/
)
execute_process(
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/monaco-editor
   ${CMAKE_SOURCE_DIR}/etc/monaco-editor/monaco-editor
)
execute_process(
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/monaco-editor.html
    ${CMAKE_SOURCE_DIR}/etc/monaco-editor
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/qwebchannel.js
    ${CMAKE_SOURCE_DIR}/etc/monaco-editor
)

