# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.
cmake_minimum_required(VERSION 3.16)

project(stream_id)

set(JRTC_ROUTER_STREAM_ID_LIB jrtc_router_stream_id)

set(JRTC_ROUTER_STREAM_ID_SRC_DIR ${PROJECT_SOURCE_DIR})

set(JRTC_ROUTER_STREAM_ID_SOURCES ${PROJECT_SOURCE_DIR}/jrtc_router_stream_id.c ${PROJECT_SOURCE_DIR}/jrtc_router_stream_id_int.c)

set(JRTC_ROUTER_STREAM_ID_HEADER_FILES ${JRTC_ROUTER_STREAM_ID_SRC_DIR} PARENT_SCOPE)

# shared and static libraries built from the same object files
add_library(jrtc_router_stream_id_static ${JRTC_ROUTER_STREAM_ID_SOURCES})
add_library(${JRTC_ROUTER_STREAM_ID_LIB} SHARED $<TARGET_OBJECTS:jrtc_router_stream_id_static>)

add_library(Jrtc::router_stream_id_lib ALIAS ${JRTC_ROUTER_STREAM_ID_LIB})
add_library(Jrtc::router_stream_id_lib_static ALIAS jrtc_router_stream_id_static)

#set_property(TARGET jrtc_router_stream_id_static PROPERTY POSITION_INDEPENDENT_CODE 1)

target_link_libraries(${JRTC_ROUTER_STREAM_ID_LIB} PUBLIC Jrtc::logger_lib)

message(STATUS JBPF_IO_HEADER_FILES  ${JBPF_IO_HEADER_FILES})

target_include_directories(${JRTC_ROUTER_STREAM_ID_LIB} PUBLIC ${JRTC_ROUTER_STREAM_ID_HEADER_FILES} 
                                                                 ${JRTC_LOGGER_HEADERS}
                                                                 ${JBPF_IO_HEADER_FILES})
target_include_directories(jrtc_router_stream_id_static PUBLIC ${JRTC_ROUTER_STREAM_ID_HEADER_FILES} 
                                                                ${JRTC_LOGGER_HEADERS}
                                                                ${JBPF_IO_HEADER_FILES})

set_target_properties(${JRTC_ROUTER_STREAM_ID_LIB}
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY  "${OUTPUT_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY  "${OUTPUT_DIR}/lib"
)

set_target_properties(jrtc_router_stream_id_static
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY  "${OUTPUT_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY  "${OUTPUT_DIR}/lib"
)

add_custom_command(TARGET ${JRTC_ROUTER_STREAM_ID_LIB} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR}/inc/ 
  COMMAND ${CMAKE_COMMAND} -E copy  ${JRTC_ROUTER_STREAM_ID_SRC_DIR}/jrtc_router_bitmap.h ${OUTPUT_DIR}/inc/  
  COMMAND ${CMAKE_COMMAND} -E copy  ${JRTC_ROUTER_STREAM_ID_SRC_DIR}/jrtc_router_stream_id.h ${OUTPUT_DIR}/inc/
  COMMAND ${CMAKE_COMMAND} -E copy  ${JRTC_ROUTER_STREAM_ID_SRC_DIR}/jrtc_router_stream_id_int.h ${OUTPUT_DIR}/inc/
)

add_cppcheck(
  ${JRTC_ROUTER_STREAM_ID_LIB}
  ${JRTC_ROUTER_STREAM_ID_SOURCES}
)

add_clang_format_check(
  ${JRTC_ROUTER_STREAM_ID_LIB}
  ${JRTC_ROUTER_STREAM_ID_SOURCES}
)
