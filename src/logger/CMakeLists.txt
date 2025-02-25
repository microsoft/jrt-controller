# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.
cmake_minimum_required(VERSION 3.16)

project(jrtc_logger)

set(JRTC_LOGGER_LIB jrtc_logger)

set(JRTC_LOGGER_SRC_DIR ${PROJECT_SOURCE_DIR})

set(JRTC_LOGGER_SOURCES ${JRTC_LOGGER_SRC_DIR}/jrtc_logging.c)
set(JRTC_LOGGER_HEADERS ${JRTC_LOGGER_SRC_DIR} PARENT_SCOPE)

add_library(${JRTC_LOGGER_LIB} ${JRTC_LOGGER_SOURCES})
add_library(Jrtc::logger_lib ALIAS ${JRTC_LOGGER_LIB})
target_include_directories(${JRTC_LOGGER_LIB} PUBLIC ${JRTC_LOGGER_HEADERS})
set_target_properties(${JECIMA_LOGGER_LIB} PROPERTIES POSITION_INDEPENDENT_CODE ON)

## add fpic
set_property(TARGET ${JRTC_LOGGER_LIB} PROPERTY POSITION_INDEPENDENT_CODE ON)

set_target_properties(${JRTC_LOGGER_LIB}
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY  "${OUTPUT_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY  "${OUTPUT_DIR}/lib"
)

add_cppcheck(
  ${JRTC_LOGGER_LIB}
  ${JRTC_LOGGER_SOURCES}
)

add_clang_format_check(
  ${JRTC_LOGGER_LIB}
  ${JRTC_LOGGER_SOURCES}
)