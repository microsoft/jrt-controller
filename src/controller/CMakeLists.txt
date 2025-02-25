# Copyright (c) Microsoft Corporation. All rights reserved.
cmake_minimum_required(VERSION 3.16)

project(controller)

find_package(Python COMPONENTS Interpreter Development REQUIRED)

include_directories(${Python_INCLUDE_DIRS})

set(JRTC_COMMON_INCLUDE_DIR ${JRTC_LOGGER_HEADERS}
  ${JRTC_ROUTER_HEADER_FILES}
  ${JRTC_ROUTER_STREAM_ID_HEADER_FILES}
  ${JBPF_IO_HEADER_FILES}
  ${JBPF_MEM_MGMT_HEADER_FILES}
  ${JRTC_REST_API_HEADER_FILES})

set(JRTC_COMMON_LINK_DIR ${JBPF_IO_LIB}
Jrtc::router_stream_id_lib_static
Jrtc::router_lib
Jrtc::logger_lib
m
${Python_LIBRARIES})

### jrtc_lib
set(JRTC_LIB jrtc_lib)

set(JRTC_LIB_SRC_DIR ${PROJECT_SOURCE_DIR})

set(JRTC_LIB_SOURCES ${JRTC_LIB_SRC_DIR}/jrtc_sched.c
                              ${JRTC_LIB_SRC_DIR}/jrtc.c
                              ${JRTC_LIB_SRC_DIR}/jrtc_int.c)

set(JRTC_LIB_HEADER_FILES ${JRTC_LIB_SOURCES})

add_library(${JRTC_LIB} SHARED ${JRTC_LIB_SOURCES})

target_include_directories(${JRTC_LIB} PUBLIC PUBLIC ${JRTC_CONTROLLER_HEADER_FILES}
                                                        ${JRTC_COMMON_INCLUDE_DIR})

target_link_options(${JRTC_LIB} PUBLIC ${Python3_LINK_OPTIONS} "-lpthread" "-ldl" "-lrt")

target_link_libraries(${JRTC_LIB} PUBLIC
                       debug ${JRTC_REST_API_LIB_DEBUG}
                       optimized ${JRTC_REST_API_LIB}
                       ${JRTC_COMMON_LINK_DIR})

add_dependencies(${JRTC_LIB} rest_api_lib)

set_property(TARGET ${JRTC_LIB} PROPERTY ENABLE_EXPORTS ON)

set_target_properties(${JRTC_LIB} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR}/bin
)

add_clang_format_check(
  ${JRTC_LIB}
  ${JRTC_LIB_SOURCES}
)

add_cppcheck(
  ${JRTC_LIB}
  ${JRTC_LIB_SOURCES}
)

#### NORTH_IO_LIB
set(JRTC_NORTH_IO_LIB jrtc_north_io)
set(JRTC_NORTH_IO_SRC ${PROJECT_SOURCE_DIR})

set(JRTC_NORTH_IO_SOURCES ${JRTC_NORTH_IO_SRC}/jrtc_north_io_app.c)

add_library(${JRTC_NORTH_IO_LIB} SHARED ${JRTC_NORTH_IO_SOURCES})
target_include_directories(${JRTC_NORTH_IO_LIB} PUBLIC ${JRTC_CONTROLLER_HEADER_FILES}
                                                         ${JRTC_COMMON_INCLUDE_DIR})

target_link_libraries(${JRTC_NORTH_IO_LIB} jbpf::io_lib)

set_target_properties(${JRTC_NORTH_IO_LIB} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIR}/lib
)

add_cppcheck(
  ${JRTC_NORTH_IO_LIB}
  ${JRTC_NORTH_IO_SOURCES}
)

add_clang_format_check(
  ${JRTC_NORTH_IO_LIB}
  ${JRTC_NORTH_IO_SOURCES}
)

## jrtc

set(JRTC_CONTROLLER jrtc)

set(JRTC_CONTROLLER_SRC_DIR ${PROJECT_SOURCE_DIR})

set(JRTC_CONTROLLER_SOURCES ${JRTC_CONTROLLER_SRC_DIR}/jrtc_sched.c
                              ${JRTC_CONTROLLER_SRC_DIR}/jrtc.c
                              ${JRTC_CONTROLLER_SRC_DIR}/jrtc_int.c)

set(JRTC_CONTROLLER_HEADER_FILES ${JRTC_CONTROLLER_SRC_DIR})

add_executable(${JRTC_CONTROLLER} ${JRTC_CONTROLLER_SOURCES})

target_include_directories(${JRTC_CONTROLLER} PUBLIC PUBLIC ${JRTC_CONTROLLER_HEADER_FILES}
                                                        ${JRTC_COMMON_INCLUDE_DIR})

message(STATUS "Python_EXECUTABLE: ${Python_EXECUTABLE}")
message(STATUS "Python_VERSION: ${Python_VERSION}")
message(STATUS "Python_INCLUDE_DIRS: ${Python_INCLUDE_DIRS}")
message(STATUS "Python_LIBRARIES: ${Python_LIBRARIES}")
set(Python3_LINK_OPTIONS "-lpython${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}")
message(STATUS "Python3_LINK_OPTIONS: ${Python3_LINK_OPTIONS}")

target_link_options(${JRTC_CONTROLLER} PUBLIC ${Python3_LINK_OPTIONS} "-lpthread" "-ldl" "-lrt")

target_link_libraries(${JRTC_CONTROLLER} PUBLIC
                       debug ${JRTC_REST_API_LIB_DEBUG}
                       optimized ${JRTC_REST_API_LIB}
                       ${JRTC_COMMON_LINK_DIR})

add_dependencies(${JRTC_CONTROLLER} ${JRTC_LIB})

set_property(TARGET ${JRTC_CONTROLLER} PROPERTY ENABLE_EXPORTS ON)

set_target_properties(${JRTC_CONTROLLER} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR}/bin
)

add_custom_command(TARGET ${JRTC_CONTROLLER} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR}/inc/
  COMMAND ${CMAKE_COMMAND} -E copy  ${JRTC_CONTROLLER_SRC_DIR}/jrtc.h ${OUTPUT_DIR}/inc/
  COMMAND ${CMAKE_COMMAND} -E copy  ${JRTC_CONTROLLER_SRC_DIR}/jrtc_int.h ${OUTPUT_DIR}/inc/
  COMMAND ${CMAKE_COMMAND} -E copy  ${JRTC_CONTROLLER_SRC_DIR}/jrtc_sched.h ${OUTPUT_DIR}/inc/
)

add_cppcheck(
  ${JRTC_CONTROLLER}
  ${JRTC_CONTROLLER_SOURCES}
)

add_clang_format_check(
  ${JRTC_CONTROLLER}
  ${JRTC_CONTROLLER_SOURCES}
)
