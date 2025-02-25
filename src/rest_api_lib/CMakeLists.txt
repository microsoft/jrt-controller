make_minimum_required(VERSION 3.16)

cmake_minimum_required(VERSION 3.16)

project(rest_api)

set(JRTC_REST_API_HEADER_FILES ${PROJECT_SOURCE_DIR}/src PARENT_SCOPE)

ExternalProject_Add(
    rest_api_lib
    DOWNLOAD_COMMAND ""
    CONFIGURE_COMMAND ""
    BUILD_COMMAND cargo build COMMAND cargo build --release --lib
    BUILD_ALWAYS true
    BINARY_DIR "${PROJECT_SOURCE_DIR}"
    INSTALL_COMMAND ""
    STEP_TARGETS update
    LOG_BUILD ON)

set(JRTC_REST_API_LIB_DEBUG "${PROJECT_SOURCE_DIR}/target/debug/libjrtc_rest_api_lib.a" PARENT_SCOPE)
set(JRTC_REST_API_LIB "${PROJECT_SOURCE_DIR}/target/release/libjrtc_rest_api_lib.a" PARENT_SCOPE)
