# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.
cmake_minimum_required(VERSION 3.16)

include(ExternalProject)
project(jrtc-tests)
include(CTest)

set(JRTC_TESTS ${JRTC_TESTS} PARENT_SCOPE)
set(TESTS_BASE ${PROJECT_SOURCE_DIR})

### Add tests
add_subdirectory(agent)
add_subdirectory(unit_tests)

add_subdirectory(router)
add_subdirectory(controller)

set(JRTC_TESTS ${JRTC_TESTS} PARENT_SCOPE)
