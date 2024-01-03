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
list(APPEND LEVEL2_SUBSYSTEMS_BIN Programmer)
list(APPEND LEVEL2_SUBSYSTEMS_NO_BIN HardwareManager ModelConfig)
list(APPEND LEVEL3_SUBSYSTEMS CFGCompiler)

# Only change this if you know what it does
list(APPEND PRE-SUBSYSTEMS ${LEVEL1_SUBSYSTEMS})
list(APPEND LIB-SUBSYSTEMS-BIN ${LEVEL2_SUBSYSTEMS_BIN})
list(APPEND LIB-SUBSYSTEMS-NO-BIN ${LEVEL2_SUBSYSTEMS_NO_BIN})
list(APPEND LIB-SUBSYSTEMS-ALL ${LIB-SUBSYSTEMS-BIN} ${LIB-SUBSYSTEMS-NO-BIN})
list(APPEND POST-SUBSYSTEMS ${LIB-SUBSYSTEMS-ALL} ${LEVEL3_SUBSYSTEMS})
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
	target_include_directories(${SUBSYSTEM} BEFORE PUBLIC
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
		list(FIND LIB-SUBSYSTEMS-BIN ${SUBSYSTEM} subsystem_index)	
		string(TOLOWER ${SUBSYSTEM} SUBSYSTEM)
		set_property(TARGET ${SUBSYSTEM} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
		set_property(TARGET ${SUBSYSTEM} PROPERTY COMPILER_FLAGS /DSTATIC_BUILD)
		set_target_properties(${SUBSYSTEM} PROPERTIES
			COMPILE_OPTIONS "$<$<CONFIG:Debug>:/MTd>$<$<CONFIG:Release>:/MT>"
		)
		if (${subsystem_index} GREATER -1)
			# Currently only ${LIB-SUBSYSTEMS-BIN} has foedag_xxxx
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
#  at top-level CMakeLists.txt, we can define configuration dependencies
#	 this default dependencis make sure all the subsystem inherit configurationRS dependencies
foreach(SUBSYSTEM ${SUBSYSTEMS})
	string(TOLOWER ${SUBSYSTEM} SUBSYSTEM)
	add_dependencies(configuration ${SUBSYSTEM})
endforeach()

# default dependencies on configuraiton (within configuration)
foreach(SUBSYSTEM ${SUBSYSTEMS})
	string(TOLOWER ${SUBSYSTEM} SUBSYSTEM)
	# Level 1
	if (NOT SUBSYSTEM STREQUAL "cfgcommon")
		add_dependencies(${SUBSYSTEM} cfgcommon)
		target_link_libraries(${SUBSYSTEM} INTERFACE cfgcommon)
		# Level 2
		if (NOT SUBSYSTEM STREQUAL "cfgcompiler")
			add_dependencies(cfgcompiler ${SUBSYSTEM})
			target_link_libraries(cfgcompiler INTERFACE ${SUBSYSTEM})
		endif()
	endif()
endforeach()

# Custom dependencies
#  This is custom dependencies within configuration subsystem
add_dependencies(cfgcompiler tcl_stubb_build)
add_dependencies(programmer tcl_stubb_build)
add_dependencies(modelconfig nlohmann_json)
