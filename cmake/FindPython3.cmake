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

set(PYTHON3_FIND_PATH "C:/msys64/mingw64")
file(GLOB potential_python3_libs "${PYTHON3_FIND_PATH}/lib/libpython3.*.dll.a")
if (potential_python3_libs)
  list (GET potential_python3_libs 0 Python3_LIBRARIES)
  if (Python3_LIBRARIES AND EXISTS ${Python3_LIBRARIES})
    get_filename_component(python3_filename ${Python3_LIBRARIES} NAME)
    string(REGEX REPLACE "libpython" "python" python3_filename ${python3_filename})
    string(REGEX REPLACE ".dll.a" "" python3_filename ${python3_filename})
    set(Python3_INCLUDE_DIRS ${PYTHON3_FIND_PATH}/include/${python3_filename})
    set(Python3_RUNTIME_LIBRARY_DIRS ${PYTHON3_FIND_PATH}/bin)
    set(Python3_EXECUTABLE ${Python3_RUNTIME_LIBRARY_DIRS}/${python3_filename}.exe)
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