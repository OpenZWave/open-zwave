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

cmake_minimum_required(VERSION 3.5)
project(open-zwave-cmake)

# Set the output directory of the openzwave library.
set(OPENZWAVE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# Compiler options
set(CMAKE_CXX_STANDARD 11)
if (MSVC)
    add_compile_options(/bigobj /permissive- /W4 /MP)
    if (CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(/O2)
    endif ()
else (MSVC)
    add_compile_options(-Wall -Wextra -Wpedantic -Wconversion)
    if (CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-O2)
    endif ()
endif (MSVC)

add_subdirectory(${PROJECT_SOURCE_DIR}/include/open-zwave)