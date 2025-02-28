# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.
project(python_loader C)
set(PYTHON_LOADER jrtc_pythonapp_loader)

# Find Python3 interpreter and libraries
find_package(Python3 REQUIRED COMPONENTS Interpreter Development)

# ctypesgen is required for generating Python bindings (from C headers)
find_program(CTYPESGEN_PATH ctypesgen)

# Check if ctypesgen was found
if(NOT CTYPESGEN_PATH)
  message(FATAL_ERROR "ctypesgen not found. Please make sure it is installed and available in your PATH.")
endif()

set(CTYPESGEN_COMMAND "${CTYPESGEN_PATH}")

set(JRTC_PATH $ENV{JRTC_PATH})
set(JRTC_OUT_DIR $ENV{JRTC_OUT_DIR})
set(NANO_PB ${JRTC_PATH}/jbpf-protobuf/3p/nanopb/)

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

# Set dependencies
set(PYTHON_LOADER_DEPENDENCIES jrtc_router_stream_id jrtc_router jrtc jrtc_logger jrtc_agent)

# Set output file name
set(PYTHON_BINDING_OUTPUT_FILE "jrtc_bindings.py")

# Source files
set(PYTHON_LOADER_SRC jrtc_pythonapp_loader.c)

# Add shared library target
add_library(${PYTHON_LOADER} SHARED ${PYTHON_LOADER_SRC})

# Link Python3 library
target_link_libraries(${PYTHON_LOADER} ${Python3_LIBRARIES} ${PYTHON_LOADER_DEPENDENCIES})

# Add dependencies
add_dependencies(${PYTHON_LOADER} generate ${PYTHON_LOADER_DEPENDENCIES})

set_target_properties(${PYTHON_LOADER} PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DIR}/lib"
  LIBRARY_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_DIR}/lib"
  LIBRARY_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DIR}/lib"
)

if(CMAKE_BUILD_TYPE MATCHES AddressSanitizer)
  # Dynamically find the ASan runtime library
  execute_process(
    COMMAND gcc -print-file-name=libasan.so
    OUTPUT_VARIABLE ASAN_LIBRARY_PATH
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  # Set LD_PRELOAD to preload ASan runtime
  set(ENV{LD_PRELOAD} ${ASAN_LIBRARY_PATH})

  message(STATUS "AddressSanitizer enabled. Preloading ASan runtime from: ${ASAN_LIBRARY_PATH}")

  # This is to fix: ==18935==ASan runtime does not come first in initial library list; you should either link runtime to your application or manually preload it with LD_PRELOAD.
  # And we don't want to see leaking memory errors from the ctypesgen
  add_custom_command(
    OUTPUT ${PYTHON_BINDING_OUTPUT_FILE}
    COMMAND ${CMAKE_COMMAND} -E env LD_PRELOAD=${ASAN_LIBRARY_PATH} ASAN_OPTIONS="detect_leaks=0" ${CTYPESGEN_COMMAND}
    -l${JRTC_OUT_DIR}/lib/libjrtc_router_stream_id.so
    ${JRTC_PATH}/src/router/jrtc_router_app_api.h
    ${JRTC_PATH}/src/controller/jrtc.h
    ${JRTC_PATH}/src/router/jrtc_router_int.h
    ${JRTC_PATH}/src/router/jrtc_router.h
    -I${NANO_PB}
    -I${JRTC_PATH}/src/router
    -I${JRTC_PATH}/src/logger
    -I${JRTC_PATH}/jbpf-protobuf/jbpf/src/io
    -I${JRTC_PATH}/jbpf-protobuf/jbpf/src/mem_mgmt
    -I${JRTC_PATH}/src/controller
    -D__GNUC__=1
    -D__extension__=
    -D__const=const
    -o ${PYTHON_BINDING_OUTPUT_FILE}
    DEPENDS
    ${PYTHON_LOADER_DEPENDENCIES}
    ${JRTC_PATH}/src/router/jrtc_router_app_api.h
    ${JRTC_PATH}/src/controller/jrtc.h
    ${JRTC_PATH}/src/router/jrtc_router_int.h
    ${JRTC_PATH}/src/router/jrtc_router.h
    COMMENT "Generating ${PYTHON_BINDING_OUTPUT_FILE}"
  )
else()
  add_custom_command(
    OUTPUT ${PYTHON_BINDING_OUTPUT_FILE}
    COMMAND ${CTYPESGEN_COMMAND}    
    -l${JRTC_OUT_DIR}/lib/libjrtc_router_stream_id.so
    ${JRTC_PATH}/src/router/jrtc_router_app_api.h
    ${JRTC_PATH}/src/controller/jrtc.h
    ${JRTC_PATH}/src/router/jrtc_router_int.h
    ${JRTC_PATH}/src/router/jrtc_router.h
    -I${NANO_PB}
    -I${JRTC_PATH}/src/router
    -I${JRTC_PATH}/src/logger
    -I${JRTC_PATH}/jbpf-protobuf/jbpf/src/io
    -I${JRTC_PATH}/jbpf-protobuf/jbpf/src/mem_mgmt
    -I${JRTC_PATH}/src/controller
    -D__GNUC__=1
    -D__extension__=
    -D__const=const
    -o ${PYTHON_BINDING_OUTPUT_FILE}
    DEPENDS
    ${PYTHON_LOADER_DEPENDENCIES}
    ${JRTC_PATH}/src/router/jrtc_router_app_api.h
    ${JRTC_PATH}/src/controller/jrtc.h
    ${JRTC_PATH}/src/router/jrtc_router_int.h
    ${JRTC_PATH}/src/router/jrtc_router.h
    COMMENT "Generating ${PYTHON_BINDING_OUTPUT_FILE}"
  )
endif()

# Add custom target for generate
add_custom_target(generate DEPENDS jrtc_bindings.py ${PYTHON_LOADER_DEPENDENCIES})

# Custom clean target to remove generated files
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${PYTHON_BINDING_OUTPUT_FILE})

## copy the libpython_loader.so to out directory
add_custom_command(TARGET ${PYTHON_LOADER} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PYTHON_LOADER}> ${OUTPUT_DIR}/lib/
)

## copy python binding to out directory
add_custom_command(TARGET ${PYTHON_LOADER} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${PYTHON_BINDING_OUTPUT_FILE} ${OUTPUT_DIR}/lib/
)

add_cppcheck(
  ${PYTHON_LOADER}
  ${PYTHON_LOADER_SRC}
)

add_clang_format_check(
  ${PYTHON_LOADER}
  ${PYTHON_LOADER_SRC}
)
