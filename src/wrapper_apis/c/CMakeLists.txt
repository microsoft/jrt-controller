# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.

cmake_minimum_required(VERSION 3.16)

project(jrtc_app)

set(JRTC_APPLIB jrtc_app)

set(JRTC_APPSRC_DIR ${PROJECT_SOURCE_DIR})

set(JRTC_APPSOURCES ${JRTC_APPSRC_DIR}/jrtc_app.cpp ${JRTC_APPSRC_DIR}/jrtc_app.hpp ${JRTC_APPSRC_DIR}/jrtc_app.h)

set(JRTC_APPHEADER_FILES ${JRTC_APPSRC_DIR})

# Add shared library target
add_library(${JRTC_APPLIB}_shared SHARED ${JRTC_APPSOURCES})

# add fPic
set_property(TARGET ${JRTC_APPLIB}_shared PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(${JRTC_APPLIB}_shared jrtc_lib Jrtc::router_stream_id_lib_static Jrtc::router_lib)

target_include_directories(${JRTC_APPLIB}_shared PUBLIC ${JRTC_APPHEADER_FILES}
                                                        ${JRTC_CONTROLLER_HEADER_FILES}
                                                        ${JRTC_ROUTER_HEADER_FILES}
                                                        ${JRTC_ROUTER_STREAM_ID_HEADER_FILES}
                                                        ${JBPF_COMMON_HEADERS}
                                                        ${JBPF_IO_HEADER_FILES}
                                                        ${JBPF_MEM_MGMT_HEADER_FILES}
                                                        )

set_target_properties(${JRTC_APPLIB}_shared
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY  "${OUTPUT_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY  "${OUTPUT_DIR}/lib"
    OUTPUT_NAME "jrtc_app"
)

add_custom_command(TARGET ${JRTC_APPLIB}_shared POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR}/inc/ 
  COMMAND ${CMAKE_COMMAND} -E copy  ${JRTC_APPSRC_DIR}/jrtc_app.h ${OUTPUT_DIR}/inc/  
)

add_cppcheck(
  ${JRTC_APPLIB}_shared
  ${JRTC_APPSOURCES}
)

add_clang_format_check(
  ${JRTC_APPLIB}_shared
  ${JRTC_APPSOURCES}
)