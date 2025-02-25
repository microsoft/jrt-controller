# Copyright (c) Microsoft Corporation. All rights reserved.
cmake_minimum_required(VERSION 3.16)

project(router)

set(JRTC_ROUTER_LIB jrtc_router)

set(JRTC_ROUTER_SRC_DIR ${PROJECT_SOURCE_DIR})

set(JRTC_ROUTER_SOURCES ${JRTC_ROUTER_SRC_DIR}/jrtc_router.c)

set(JRTC_ROUTER_HEADER_FILES ${JRTC_ROUTER_SRC_DIR} PARENT_SCOPE)

# Add static library
add_library(${JRTC_ROUTER_LIB} STATIC ${JRTC_ROUTER_SOURCES})

## add fPic
set_property(TARGET ${JRTC_ROUTER_LIB} PROPERTY POSITION_INDEPENDENT_CODE ON)

add_library(Jrtc::router_lib ALIAS ${JRTC_ROUTER_LIB})

target_link_libraries(${JRTC_ROUTER_LIB} Jrtc::router_stream_id_lib_static
                                            jbpf::io_lib)

target_include_directories(${JRTC_ROUTER_LIB} PUBLIC ${JRTC_ROUTER_HEADER_FILES}
                                                       ${JRTC_LOGGER_HEADERS}
                                                       ${JRTC_ROUTER_STREAM_ID_HEADER_FILES}
                                                       ${JBPF_IO_HEADER_FILES}
                                                       ${JBPF_MEM_MGMT_HEADER_FILES}
                                                       ${JBPF_CK_HEADER_FILES})

add_dependencies(${JRTC_ROUTER_LIB} jbpf_libck)

set_target_properties(${JRTC_ROUTER_LIB}
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY  "${OUTPUT_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY  "${OUTPUT_DIR}/lib"
)

add_custom_command(TARGET ${JRTC_ROUTER_LIB} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR}/inc/ 
  COMMAND ${CMAKE_COMMAND} -E copy  ${JRTC_ROUTER_SRC_DIR}/jrtc_router_app_api.h ${OUTPUT_DIR}/inc/  
)

# Add shared library target
add_library(${JRTC_ROUTER_LIB}_shared SHARED ${JRTC_ROUTER_SOURCES})
add_library(Jrtc::router_lib_shared ALIAS ${JRTC_ROUTER_LIB}_shared)

# add fPic
set_property(TARGET ${JRTC_ROUTER_LIB}_shared PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(${JRTC_ROUTER_LIB}_shared Jrtc::router_stream_id_lib_static
                                                  jbpf::io_lib)

target_include_directories(${JRTC_ROUTER_LIB}_shared PUBLIC ${JRTC_ROUTER_HEADER_FILES}
                                                           ${JRTC_LOGGER_HEADERS}
                                                           ${JRTC_ROUTER_STREAM_ID_HEADER_FILES}
                                                           ${JBPF_IO_HEADER_FILES}
                                                           ${JBPF_MEM_MGMT_HEADER_FILES}
                                                           ${JBPF_CK_HEADER_FILES})

add_dependencies(${JRTC_ROUTER_LIB}_shared jbpf_libck)

set_target_properties(${JRTC_ROUTER_LIB}_shared
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY  "${OUTPUT_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY  "${OUTPUT_DIR}/lib"
    OUTPUT_NAME "libjrtc_router"
)

add_cppcheck(
  ${JRTC_ROUTER_LIB}
  ${JRTC_ROUTER_SOURCES}
)

add_clang_format_check(
  ${JRTC_ROUTER_LIB}
  ${JRTC_ROUTER_SOURCES}
)