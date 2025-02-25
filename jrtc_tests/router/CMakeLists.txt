# Copyright (c) Microsoft Corporation. All rights reserved.
set(ROUTER_TESTS ${TESTS_BASE}/router)
file(GLOB ROUTER_TESTS_SOURCES ${ROUTER_TESTS}/*.c)
set(JRTC_TESTS ${JRTC_TESTS} PARENT_SCOPE)
# Loop through each test file and create an executable
foreach(TEST_FILE ${ROUTER_TESTS_SOURCES})
  # Get the filename without the path
  get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)

  # Create an executable target for the test
  add_executable(${TEST_NAME} ${TEST_FILE}) 

  # Link the necessary libraries
  target_link_libraries(${TEST_NAME} PUBLIC jrtc_agent Jrtc::logger_lib Jrtc::router_stream_id_lib Jrtc::router_lib)

  # Set the include directories
  target_include_directories(${TEST_NAME} PUBLIC ${JRTC_ROUTER_HEADER_FILES})

  # Add the test to the list of tests to be executed
  add_test(NAME router/${TEST_NAME} COMMAND ${TEST_NAME})

  # Test coverage
  list(APPEND JRTC_TESTS router/${TEST_NAME})
  add_clang_format_check(${TEST_NAME} ${TEST_FILE})
  add_cppcheck(${TEST_NAME} ${TEST_FILE})
  set(JRTC_TESTS ${JRTC_TESTS} PARENT_SCOPE)
endforeach()
