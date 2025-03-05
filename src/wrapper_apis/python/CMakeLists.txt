# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.

cmake_minimum_required(VERSION 3.16)

project(jrtc_python_copy)

set(JRTC_APPSRC_DIR ${PROJECT_SOURCE_DIR})

# Copy Python files after the build step
add_custom_target(copy_python_files ALL
  COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR}/lib/ 
  COMMAND ${CMAKE_COMMAND} -E copy ${JRTC_APPSRC_DIR}/jrtc_app.py ${OUTPUT_DIR}/lib/
  COMMAND ${CMAKE_COMMAND} -E copy ${JRTC_APPSRC_DIR}/jrtc_router_lib.py ${OUTPUT_DIR}/lib/
  COMMAND ${CMAKE_COMMAND} -E copy ${JRTC_APPSRC_DIR}/jrtc_router_stream_id.py ${OUTPUT_DIR}/lib/
  COMMAND ${CMAKE_COMMAND} -E copy ${JRTC_APPSRC_DIR}/jrtc_wrapper_utils.py ${OUTPUT_DIR}/lib/
)