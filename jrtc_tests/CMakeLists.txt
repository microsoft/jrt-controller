cmake_minimum_required(VERSION 3.16)

include(ExternalProject)
project(jrtc-tests)
include(CTest)

set(JRTC_TESTS ${JRTC_TESTS} PARENT_SCOPE)
set(TESTS_BASE ${PROJECT_SOURCE_DIR})

### Add tests
add_subdirectory(agent)
add_subdirectory(unit_tests)


## TODO: These tests are flakey and need to be fixed
if(CMAKE_BUILD_TYPE MATCHES AddressSanitizer)
    message(STATUS "TOFIX: AddressSanitizer enabled, skipping: router, controller flakey tests")
else()
    add_subdirectory(router)
    add_subdirectory(controller)
endif()

set(JRTC_TESTS ${JRTC_TESTS} PARENT_SCOPE)
