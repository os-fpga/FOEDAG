# - Try to find the freetype library
# Once done this defines
#
#  Python3_LIBRARIES - fullpath of the python3 library
#  Python3_INCLUDE_DIRS - include folder of python3
#  Python3_RUNTIME_LIBRARY_DIRS - bin folder of python3
#  Python3_EXECUTABLE - exe of python3 
#  Python3_FOUND - set when all four are found

#  Work for msys2 only

# Copyright (c) 2024 Chai, Chung Shien
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
message("FindCustomPython3 MINGW: ${MINGW}")
message("FindCustomPython3 MSVC: ${MSVC}")
message("FindCustomPython3 WIN32: ${WIN32}")
set(PYTHON3_FIND_PATH "C:/msys64/mingw64")
if ((MINGW OR (NOT MSVC AND WIN32)) AND EXISTS ${PYTHON3_FIND_PATH})
  file(GLOB potential_python3_libs "${PYTHON3_FIND_PATH}/lib/libpython3.*.dll.a")
  if (potential_python3_libs)
    list (GET potential_python3_libs 0 Python3_LIBRARIES)
    if (Python3_LIBRARIES AND EXISTS ${Python3_LIBRARIES})
      get_filename_component(python3_filename ${Python3_LIBRARIES} NAME)
      string(REGEX REPLACE "libpython" "python" python3_filename ${python3_filename})
      string(REGEX REPLACE ".dll.a" "" python3_filename ${python3_filename})
      set(Python3_EXECUTABLE ${PYTHON3_FIND_PATH}/bin/${python3_filename}.exe)
      set(Python3_INCLUDE_DIRS ${PYTHON3_FIND_PATH}/include/${python3_filename})
      set(Python3_LIBRARY_DIRS ${PYTHON3_FIND_PATH}/lib/${python3_filename})
      set(Python3_RUNTIME_LIBRARY_DIRS ${PYTHON3_FIND_PATH}/lib/${python3_filename})
      set(Python3_PY_LIBRARY_DIR ${Python3_LIBRARY_DIRS})
      if (Python3_INCLUDE_DIRS AND EXISTS ${Python3_INCLUDE_DIRS})
        if (Python3_RUNTIME_LIBRARY_DIRS AND EXISTS ${Python3_RUNTIME_LIBRARY_DIRS})
          if (Python3_EXECUTABLE AND EXISTS ${Python3_EXECUTABLE})
            set(Python3_FOUND TRUE)
          else()
            message(SEND_ERROR "${Python3_EXECUTABLE} does not exist")
          endif()
        else()
          message(SEND_ERROR "${Python3_RUNTIME_LIBRARY_DIRS} does not exist")
        endif()
      else()
        message(SEND_ERROR "${Python3_INCLUDE_DIRS} does not exist")
      endif()
    else()
      message(SEND_ERROR "Fail to look for libpython3.*.dll.a in ${PYTHON3_FIND_PATH}/lib")
    endif()
  else()
    message(SEND_ERROR "Fail to look for libpython3.*.dll.a in ${PYTHON3_FIND_PATH}/lib")
  endif()
else()
  find_package(Python3 3.3 REQUIRED COMPONENTS Interpreter Development)
  if (TRUE)
    if (Python3_FOUND)
      get_filename_component(lib_filename ${Python3_LIBRARIES} NAME)
      string(REGEX REPLACE "/${lib_filename}" "" lib_path ${Python3_LIBRARIES})
      message(STATUS "CSCHAI PYTHON3: Lib Dir: ${lib_path}")
      message(STATUS "CSCHAI PYTHON3: Python3_VERSION: ${Python3_VERSION}")
      message(STATUS "CSCHAI PYTHON3: Python3_VERSION_MAJOR: ${Python3_VERSION_MAJOR}")
      message(STATUS "CSCHAI PYTHON3: Python3_VERSION_MINOR: ${Python3_VERSION_MINOR}")
      execute_process(COMMAND ls ${lib_path}/*python*)
      message(STATUS "CSCHAI PYTHON3: Try to find PY LIB")
      set(os_py ${Python3_LIBRARIES}/os.py)
      if (EXISTS ${os_py})
        message(STATUS "CSCHAI PYTHON3: Found ${os_py}")
      else()
        set(os_py ${Python3_LIBRARIES}/Lib/os.py)
        if (EXISTS ${os_py})
          message(STATUS "CSCHAI PYTHON3: Found ${os_py}")
        else()
          set(os_py ${Python3_LIBRARIES}/python${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}/os.py)
          if (EXISTS ${os_py})
            message(STATUS "CSCHAI PYTHON3: Found ${os_py}")
          else()
            set(os_py /usr/lib/python${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}/os.py)
            if (EXISTS ${os_py})
              message(STATUS "CSCHAI PYTHON3: Found ${os_py}")
            else()
              message(STATUS "CSCHAI PYTHON3: Fail to find PY LIB")
            endif()
          endif()
        endif()
      endif()
      #if (MSVC)
      #  string(REGEX REPLACE ".dll" ".lib" Python3_LIBRARIES ${Python3_LIBRARIES})
      #else()
      #  string(REGEX REPLACE ".so" ".a" Python3_LIBRARIES ${Python3_LIBRARIES})
      #endif()
      #if (EXISTS ${Python3_LIBRARIES})
      #  message(STATUS "Use static library ${Python3_LIBRARIES}")
      #else()
      #  message(SEND_ERROR "${Python3_LIBRARIES} does not exist")
      #endif()
      #if (EXISTS ${Python3_LIBRARY_DIRS}/__pycache__)
      #  set(Python3_PY_LIBRARY_DIR, ${Python3_LIBRARY_DIRS})
      #else()
      #  get_filename_component(python3_filename ${Python3_LIBRARIES} NAME)
      #  string(REGEX REPLACE "libpython" "python" python3_filename ${python3_filename})
      #  string(REGEX REPLACE ".a" "" python3_filename ${python3_filename})
      #  string(REGEX REPLACE ".so" "" python3_filename ${python3_filename})
      #  message(STATUS "Python Name: ${python3_filename}")
      #  set(Python3_PY_LIBRARY_DIR /usr/lib/${python3_filename})
      #  message(STATUS "Look for ${Python3_PY_LIBRARY_DIR}/__pycache__")
      #  if (EXISTS ${Python3_PY_LIBRARY_DIR}/__pycache__)
      #    message(STATUS "Python PY Library: ${Python3_PY_LIBRARY_DIR}")
      #  else()
      #    message(SEND_ERROR "Fail to look for Python PY library")
      #  endif()
      #endif()
    endif()
  endif()
endif()