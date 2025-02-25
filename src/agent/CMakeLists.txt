# Copyright (c) Microsoft Corporation. All rights reserved.
cmake_minimum_required(VERSION 3.16)

project(agent)

set(JRTC_AGENT_LIB jrtc_agent)

set(JRTC_AGENT_SRC_DIR ${PROJECT_SOURCE_DIR})
set(JRTC_AGENT_SOURCES ${JRTC_AGENT_SRC_DIR}/jrtc_agent.c)

set(JRTC_AGENT_HEADER_FILES ${JRTC_AGENT_SRC_DIR} PARENT_SCOPE)

add_library(${JRTC_AGENT_LIB} SHARED ${JRTC_AGENT_SOURCES})
target_include_directories(${JRTC_AGENT_LIB} PUBLIC ${JRTC_AGENT_HEADER_FILES}
                                                      ${JBPF_IO_HEADER_FILES}
                                                      ${JBPF_MEM_MGMT_HEADER_FILES}
                                                      ${JRTC_ROUTER_STREAM_ID_HEADER_FILES})

target_link_libraries(${JRTC_AGENT_LIB} PUBLIC jbpf::io_lib Jrtc::router_stream_id_lib)

set_target_properties(${JRTC_AGENT_LIB} PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIR}/lib
)

add_custom_command(TARGET ${JRTC_AGENT_LIB} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR}/inc/
  COMMAND ${CMAKE_COMMAND} -E copy ${JRTC_AGENT_SRC_DIR}/jrtc_agent.h ${OUTPUT_DIR}/inc/
  COMMAND ${CMAKE_COMMAND} -E copy ${JRTC_AGENT_SRC_DIR}/jrtc_agent_defs.h ${OUTPUT_DIR}/inc/
)

add_cppcheck(
  ${JRTC_AGENT_LIB}
  ${JRTC_AGENT_SOURCES}
)
add_clang_format_check(
  ${JRTC_AGENT_LIB}
  ${JRTC_AGENT_SOURCES}
)
