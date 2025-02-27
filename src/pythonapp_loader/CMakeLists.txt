# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.
project(python_loader C)
set(PYTHON_LOADER jrtc_pythonapp_loader)

# Find Python3 interpreter and libraries
find_package(Python3 REQUIRED COMPONENTS Interpreter Development)

set(JRTC_PATH $ENV{JRTC_PATH})
set(JRTC_OUT_DIR $ENV{JRTC_OUT_DIR})
set(NANO_PB $ENV{NANO_PB})

message(STATUS "JRTC_PATH: ${JRTC_PATH}")
message(STATUS "JRTC_OUT_DIR: ${JRTC_OUT_DIR}")
message(STATUS "NANO_PB: ${NANO_PB}")

# Include directories
include_directories(
    ${Python3_INCLUDE_DIRS}
    ${JRTC_PATH}/src/router
    ${JRTC_PATH}/src/stream_id
    ${JRTC_PATH}/jbpf-protobuf/jbpf/src/io
    ${JRTC_PATH}/3p/nanopb
    ${JRTC_PATH}/jbpf-protobuf/jbpf/src/common
    ${JRTC_PATH}/jbpf-protobuf/jbpf/src/mem_mgmt
    ${JRTC_PATH}/src/controller
)

# Source files
set(PYTHON_LOADER_SRC jrtc_pythonapp_loader.c)

# Add shared library target
add_library(${PYTHON_LOADER} SHARED ${PYTHON_LOADER_SRC})

# Link Python3 library
target_link_libraries(${PYTHON_LOADER} ${Python3_LIBRARIES})

# Add dependencies
add_dependencies(${PYTHON_LOADER} jrtc_router_stream_id jrtc_router jrtc jrtc_logger jrtc_agent generate)

set_target_properties(${PYTHON_LOADER} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY  "${OUTPUT_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY_DEBUG  "${OUTPUT_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY_RELEASE  "${OUTPUT_DIR}/lib"
)

# Add custom command for generating jrtc_bindings.py
if (CMAKE_BUILD_TYPE STREQUAL "AddressSanitizer")
  set(CTYPESGEN_COMMAND "sh -c 'env ASAN_OPTIONS=detect_leaks=0 LD_PRELOAD= ctypesgen'")
else()
  set(CTYPESGEN_COMMAND "ctypesgen")
endif()

add_custom_command(
    OUTPUT jrtc_bindings.py
    COMMAND ${CTYPESGEN_COMMAND}
            -l${JRTC_OUT_DIR}/lib/libjrtc_router_stream_id.so
            ${JRTC_PATH}/src/router/jrtc_router_app_api.h
            ${JRTC_PATH}/src/controller/jrtc.h
            ${JRTC_PATH}/src/router/jrtc_router_int.h
            ${JRTC_PATH}/src/router/jrtc_router.h
            -I${JRTC_OUT_DIR}/inc
            -I${NANO_PB}
            -I${JRTC_PATH}/src/router
            -I${JRTC_PATH}/src/logger
            -I${JRTC_PATH}/jbpf-protobuf/jbpf/src/io
            -I${JRTC_PATH}/jbpf-protobuf/jbpf/src/mem_mgmt
            -I${JRTC_PATH}/src/controller
            -D__GNUC__=1
            -D__extension__=
            -D__const=const
            -o jrtc_bindings.py
    DEPENDS
        ${JRTC_PATH}/src/router/jrtc_router_app_api.h
        ${JRTC_PATH}/src/controller/jrtc.h
        ${JRTC_PATH}/src/router/jrtc_router_int.h
        ${JRTC_PATH}/src/router/jrtc_router.h
    COMMENT "Generating jrtc_bindings.py"
)

# Add custom target for generate
add_custom_target(generate DEPENDS jrtc_bindings.py)

# Custom clean target to remove generated files
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "jrtc_bindings.py")

## copy the libpython_loader.so to out directory
add_custom_command(TARGET ${PYTHON_LOADER} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PYTHON_LOADER}> ${OUTPUT_DIR}/lib/
)

## copy jrtc_bindings.py to out directory
add_custom_command(TARGET ${PYTHON_LOADER} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy jrtc_bindings.py ${OUTPUT_DIR}/lib/
)

add_cppcheck(
  ${PYTHON_LOADER}
  ${PYTHON_LOADER_SRC}
)

add_clang_format_check(
  ${PYTHON_LOADER}
  ${PYTHON_LOADER_SRC}
)
