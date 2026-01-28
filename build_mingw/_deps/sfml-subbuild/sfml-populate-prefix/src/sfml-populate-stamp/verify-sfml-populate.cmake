# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

if("C:/Users/HP/Desktop/LORENZO/horrorgame/libs/sfml.zip" STREQUAL "")
  message(FATAL_ERROR "LOCAL can't be empty")
endif()

if(NOT EXISTS "C:/Users/HP/Desktop/LORENZO/horrorgame/libs/sfml.zip")
  message(FATAL_ERROR "File not found: C:/Users/HP/Desktop/LORENZO/horrorgame/libs/sfml.zip")
endif()

if("" STREQUAL "")
  message(WARNING "File cannot be verified since no URL_HASH specified")
  return()
endif()

if("" STREQUAL "")
  message(FATAL_ERROR "EXPECT_VALUE can't be empty")
endif()

message(VERBOSE "verifying file...
     file='C:/Users/HP/Desktop/LORENZO/horrorgame/libs/sfml.zip'")

file("" "C:/Users/HP/Desktop/LORENZO/horrorgame/libs/sfml.zip" actual_value)

if(NOT "${actual_value}" STREQUAL "")
  message(FATAL_ERROR "error:  hash of
  C:/Users/HP/Desktop/LORENZO/horrorgame/libs/sfml.zip
does not match expected value
  expected: ''
    actual: '${actual_value}'
")
endif()

message(VERBOSE "verifying file... done")
