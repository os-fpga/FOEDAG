cmake_minimum_required(VERSION 3.15)

set(cfg_path ${CMAKE_CURRENT_SOURCE_DIR})
project(configuration)

add_custom_target(configuration
	COMMAND
		echo "This is top level build for Configuration subsystem. It does not really build any target but it defines all subsystem or subdirectory and their dependencies"
	)

add_custom_target(pre-configuration
	COMMAND
		echo "This is top level build for Pre-Configuration subsystem. It does not really build any target but it defines all subsystem or subdirectory that need to be built in configuration first"
	)

# Define the subsystem here
list(APPEND LEVEL1_SUBSYSTEMS CFGCommon)
list(APPEND LEVEL2_SUBSYSTEMS CFGCompiler)
list(APPEND LEVEL3_SUBSYSTEMS Programmer)

# Only change this if you know what it does
list(APPEND PRE-SUBSYSTEMS ${LEVEL1_SUBSYSTEMS} ${LEVEL2_SUBSYSTEMS})
list(APPEND POST-SUBSYSTEMS ${LEVEL3_SUBSYSTEMS})
list(APPEND SUBSYSTEMS ${PRE-SUBSYSTEMS} ${POST-SUBSYSTEMS})

# set parent scope directory definition before subdirectory is added so that it can be inherited
#	in each subdirectory avoid using using ".." relative path -- it is very hard to manage if we have multiple layer
set(CFG_PROJECT_ROOT_DIR ${PROJECT_SOURCE_DIR}/../..)
set(CFG_BUILD_ROOT_DIR ${CMAKE_CURRENT_BINARY_DIR}/../..)
message("Configuration source dirctory is ${PROJECT_SOURCE_DIR}")
message("Configuration binary dirctory is ${CMAKE_CURRENT_BINARY_DIR}")
message("Configuration set project root directory as ${CFG_PROJECT_ROOT_DIR}")
message("Configuration set build root directory as ${CFG_BUILD_ROOT_DIR}")

# Determine the library type for configuration
set(CFG_LIB_TYPE STATIC)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CFG_BUILD_ROOT_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CFG_BUILD_ROOT_DIR}/bin)

# subdirectory
foreach(SUBSYSTEM ${SUBSYSTEMS})
  message("Configuration add subsystem ${SUBSYSTEM}")
	add_subdirectory(${SUBSYSTEM})
endforeach()

# include configuration to all subsystem
foreach(SUBSYSTEM ${SUBSYSTEMS})
	string(TOLOWER ${SUBSYSTEM} SUBSYSTEM)
	target_include_directories(${SUBSYSTEM} PUBLIC
				${CFG_PROJECT_ROOT_DIR}
        ${CFG_PROJECT_ROOT_DIR}/src
        ${CFG_PROJECT_ROOT_DIR}/src/Configuration
        ${CFG_BUILD_ROOT_DIR}/include
        ${CFG_BUILD_ROOT_DIR}/src
        ${CFG_BUILD_ROOT_DIR}/src/Configuration
    )
endforeach()

if(MSVC)
	foreach(SUBSYSTEM ${SUBSYSTEMS})
		list(FIND POST-SUBSYSTEMS ${SUBSYSTEM} subsystem_index)	
		string(TOLOWER ${SUBSYSTEM} SUBSYSTEM)
		set_property(TARGET ${SUBSYSTEM} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
		set_property(TARGET ${SUBSYSTEM} PROPERTY COMPILER_FLAGS /DSTATIC_BUILD)
		set_target_properties(${SUBSYSTEM} PROPERTIES
			COMPILE_OPTIONS "$<$<CONFIG:Debug>:/MTd>$<$<CONFIG:Release>:/MT>"
		)
		# Every subsystem will have test
		set_property(TARGET ${SUBSYSTEM}_test 
			PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
		)
		set_property(TARGET ${SUBSYSTEM}_test 
			PROPERTY COMPILER_FLAGS /DSTATIC_BUILD
		)
		set_target_properties(${SUBSYSTEM}_test PROPERTIES
			COMPILE_OPTIONS "$<$<CONFIG:Debug>:/MTd>$<$<CONFIG:Release>:/MT>"
		)
		if (${subsystem_index} GREATER -1)
			# Currently only ${POST-SUBSYSTEMS} has foedag_xxxx
			set_property(TARGET foedag_${SUBSYSTEM} 
				PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
			)
			set_property(TARGET foedag_${SUBSYSTEM}
				PROPERTY COMPILER_FLAGS /DSTATIC_BUILD
			)
			set_target_properties(foedag_${SUBSYSTEM} PROPERTIES
				COMPILE_OPTIONS "$<$<CONFIG:Debug>:/MTd>$<$<CONFIG:Release>:/MT>"
			)
		endif()
	endforeach()
endif()

# default dependencies on pre-configuration (within configuration itself)
foreach(SUBSYSTEM ${PRE-SUBSYSTEMS})
	string(TOLOWER ${SUBSYSTEM} SUBSYSTEM)
	add_dependencies(pre-configuration ${SUBSYSTEM})
endforeach()

# default dependencies on post-configuration (within configuration itself)
foreach(SUBSYSTEM ${POST-SUBSYSTEMS})
	string(TOLOWER ${SUBSYSTEM} SUBSYSTEM)
	add_dependencies(${SUBSYSTEM} pre-configuration)
endforeach()

# default dependencies on configuraiton (with outside world)
# 	at top-level CMakeLists.txt, we can define configuration dependencies
#   this default dependencis make sure all the subsystem inherit configurationRS dependencies
foreach(SUBSYSTEM ${SUBSYSTEMS})
	string(TOLOWER ${SUBSYSTEM} SUBSYSTEM)
	add_dependencies(configuration ${SUBSYSTEM})
endforeach()

# default dependencies on configuraiton (within configuration)
foreach(SUBSYSTEM ${SUBSYSTEMS})
	string(TOLOWER ${SUBSYSTEM} SUBSYSTEM)
	# Level 1
	if (NOT SUBSYSTEM STREQUAL "cfgcommon")
		target_link_libraries(${SUBSYSTEM} INTERFACE cfgcommon)
		# Level 2
		if (NOT SUBSYSTEM STREQUAL "cfgcompiler")
			target_link_libraries(cfgcompiler INTERFACE ${SUBSYSTEM})
		endif()
	endif()
endforeach()

# Custom dependencies
# 	This is custom dependencies within configuration subsystem
# Example BitAssembler use CFGObject library 
add_dependencies(cfgcompiler tcl_stubb_build)
