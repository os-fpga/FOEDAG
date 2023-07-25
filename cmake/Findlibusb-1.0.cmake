# - Try to find libusb-1.0
# Once done this will define
#
#  LIBUSB_1_FOUND - system has libusb
#  LIBUSB_1_INCLUDE_DIRS - the libusb include directory
#  LIBUSB_1_LIBRARIES - Link these to use libusb
#  LIBUSB_1_DEFINITIONS - Compiler switches required for using libusb
#
#  Adapted from cmake-modules Google Code project
#
#  Copyright (c) 2006 Andreas Schneider <mail@cynapses.org>
#
#  (Changes for libusb) Copyright (c) 2008 Kyle Machulis <kyle@nonpolynomial.com>
#
# Redistribution and use is allowed according to the terms of the New BSD license.
#
# CMake-Modules Project New BSD License
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
#
# * Neither the name of the CMake-Modules Project nor the names of its
#   contributors may be used to endorse or promote products derived from this
#   software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
#  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

message("Looking for libusb-1.0.............")
if (LIBUSB_1_LIBRARIES AND LIBUSB_1_INCLUDE_DIRS)
  # in cache already
  message("libusb-1.0 in cache already")
  set(LIBUSB_FOUND TRUE)
else (LIBUSB_1_LIBRARIES AND LIBUSB_1_INCLUDE_DIRS)
  IF (NOT WIN32)
    find_package(PkgConfig)  
    pkg_check_modules(TGY_LIBUSB libusb-1.0)
    message("1 TGY_LIBUSB_FOUND: ${TGY_LIBUSB_FOUND}")
    message("2 TGY_LIBUSB_LIBRARIES: ${TGY_LIBUSB_LIBRARIES}")
    message("3 TGY_LIBUSB_INCLUDE_DIRS: ${TGY_LIBUSB_INCLUDE_DIRS}")
    message("4 TGY_LIBUSB_INCLUDE_DIR: ${TGY_LIBUSB_INCLUDE_DIR}")
    message("5 TGY_LIBUSB_VERSION: ${TGY_LIBUSB_VERSION}")
    message("6 TGY_LIBUSB_LDFLAGS: ${TGY_LIBUSB_LDFLAGS}")
    message("7 TGY_LIBUSB_LDFLAGS_OTHER: ${TGY_LIBUSB_LDFLAGS_OTHER}")
    message("8 TGY_LIBUSB_CFLAGS: ${TGY_LIBUSB_CFLAGS}")
    message("9 TGY_LIBUSB_CFLAGS_OTHER: ${TGY_LIBUSB_CFLAGS_OTHER}")
    message("10 TGY_LIBUSB_LIBDIR: ${TGY_LIBUSB_LIBDIR}")
    message("LIBUSB_1_INCLUDE_DIR: ${LIBUSB_1_INCLUDE_DIR}")
    message("LIBUSB_1_LIBRARY: ${LIBUSB_1_LIBRARY}")
  ENDIF(NOT WIN32)

  find_path(LIBUSB_1_INCLUDE_DIR
    NAMES
	libusb.h
    PATHS
      /usr/include
      /usr/local/include
      /opt/local/include
      /sw/include
       /usr/local/Cellar/libusb/1.0.26/include
	PATH_SUFFIXES
	  libusb-1.0
  )

  find_library(LIBUSB_1_LIBRARY
    NAMES
      usb-1.0 usb
    PATHS
      /usr/lib
      /usr/local/lib
      /opt/local/lib
      /sw/lib
      /usr/local/Cellar/libusb/1.0.26/lib
  )

  set(LIBUSB_1_INCLUDE_DIRS
    ${LIBUSB_1_INCLUDE_DIR}
  )

  set(LIBUSB_1_LIBRARIES
    ${LIBUSB_1_LIBRARY}
)

  if (LIBUSB_1_INCLUDE_DIRS AND LIBUSB_1_LIBRARIES)
     set(LIBUSB_1_FOUND TRUE)
  endif (LIBUSB_1_INCLUDE_DIRS AND LIBUSB_1_LIBRARIES)

  if (LIBUSB_1_FOUND)
    if (NOT libusb_1_FIND_QUIETLY)
      message(STATUS "Found libusb-1.0:")
      message(STATUS " - Includes: ${LIBUSB_1_INCLUDE_DIRS}")
      message(STATUS " - Libraries: ${LIBUSB_1_LIBRARIES}")
    endif (NOT libusb_1_FIND_QUIETLY)
  else (LIBUSB_1_FOUND)
    if (libusb_1_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find libusb")
    endif (libusb_1_FIND_REQUIRED)
  endif (LIBUSB_1_FOUND)

  # show the LIBUSB_1_INCLUDE_DIRS and LIBUSB_1_LIBRARIES variables only in the advanced view
  mark_as_advanced(LIBUSB_1_INCLUDE_DIRS LIBUSB_1_LIBRARIES)

endif (LIBUSB_1_LIBRARIES AND LIBUSB_1_INCLUDE_DIRS)