###
## MIT License
##
## Copyright (c) 2017-2018 Stefan Broekman. All rights reserved.
## https://stefanbroekman.nl
##
## Permission is hereby granted, free of charge, to any person obtaining a copy
## of this software and associated documentation files (the "Software"), to deal
## in the Software without restriction, including without limitation the rights
## to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
## copies of the Software, and to permit persons to whom the Software is
## furnished to do so, subject to the following conditions:
##
## The above copyright notice and this permission notice shall be included in all
## copies or substantial portions of the Software.
##
## THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
## IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
## FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
## AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
## LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
## OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
## SOFTWARE.
###

# Path options.
set(OPENZWAVE_FILES_DIR "${PROJECT_SOURCE_DIR}/include/open-zwave")
set(HIDAPI_FILES_DIR "${PROJECT_SOURCE_DIR}/include/open-zwave/hidapi")
set(TINYXML_FILES_DIR "${PROJECT_SOURCE_DIR}/include/open-zwave/tinyxml")

# Set library output directory if not already defined.
if (NOT DEFINED OPENZWAVE_LIB_OUTPUT_DIRECTORY)
    set(OPENZWAVE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
endif ()

# OpenZWave includes files from the root of the source folder so we have to include to prevent file not found errors.
include_directories(
        SYSTEM ${OPENZWAVE_FILES_DIR}
        SYSTEM ${HIDAPI_FILES_DIR}/hidapi
        SYSTEM ${TINYXML_FILES_DIR})

# Recursively scan for all source and header files.
if (NOT EXISTS ${OPENZWAVE_FILES_DIR}/vers.c)
    file(WRITE ${OPENZWAVE_FILES_DIR}/vers.c "char ozw_vers[] = \"OpenZWave version 1.0.'`svnversion ../..`'\";")
endif ()
file(GLOB_RECURSE OPENZWAVE_FILES
        ${OPENZWAVE_FILES_DIR}/*.cpp
        ${OPENZWAVE_FILES_DIR}/*.hpp
        ${OPENZWAVE_FILES_DIR}/*.c
        ${OPENZWAVE_FILES_DIR}/*.h)
list(FILTER OPENZWAVE_FILES EXCLUDE REGEX "${OPENZWAVE_FILES_DIR}/hidapi/*")
list(FILTER OPENZWAVE_FILES EXCLUDE REGEX "${OPENZWAVE_FILES_DIR}/tinyxml/*")

file(GLOB_RECURSE HIDAPI_FILES
        ${HIDAPI_FILES_DIR}/*.cpp
        ${HIDAPI_FILES_DIR}/*.hpp
        ${HIDAPI_FILES_DIR}/*.c
        ${HIDAPI_FILES_DIR}/*.h)

file(GLOB_RECURSE TINYXML_FILES
        ${TINYXML_FILES_DIR}/*.cpp
        ${TINYXML_FILES_DIR}/*.hpp
        ${TINYXML_FILES_DIR}/*.c
        ${TINYXML_FILES_DIR}/*.h)

# Platform specific file filtering.
if (UNIX)
    if (NOT APPLE)
        list(FILTER HIDAPI_FILES EXCLUDE REGEX "${HIDAPI_FILES_DIR}/mac/*")
    endif ()
    list(FILTER OPENZWAVE_FILES EXCLUDE REGEX "${OPENZWAVE_FILES_DIR}/platform/winRT/*")
    list(FILTER OPENZWAVE_FILES EXCLUDE REGEX "${OPENZWAVE_FILES_DIR}/platform/windows/*")
    list(FILTER HIDAPI_FILES EXCLUDE REGEX "${HIDAPI_FILES_DIR}/windows/*")
elseif (WIN32)
    list(FILTER OPENZWAVE_FILES EXCLUDE REGEX "${OPENZWAVE_FILES_DIR}/platform/winRT/*") #todo: WindowsRT specific check.
    list(FILTER OPENZWAVE_FILES EXCLUDE REGEX "${OPENZWAVE_FILES_DIR}/platform/unix/*")
    list(FILTER HIDAPI_FILES EXCLUDE REGEX "${HIDAPI_FILES_DIR}/linux/*")
    list(FILTER HIDAPI_FILES EXCLUDE REGEX "${HIDAPI_FILES_DIR}/libusb/*")
    list(FILTER HIDAPI_FILES EXCLUDE REGEX "${HIDAPI_FILES_DIR}/mac/*")
endif ()

# Create a static library.
add_library(hidapi STATIC ${HIDAPI_FILES})
add_library(tinyxml STATIC ${TINYXML_FILES})
add_library(openzwave STATIC ${OPENZWAVE_FILES})
target_link_libraries(openzwave hidapi tinyxml)
set_target_properties(openzwave PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${OPENZWAVE_LIBRARY_OUTPUT_DIRECTORY}")