cmake_minimum_required(VERSION 3.16)

project(jrtc)
include(ExternalProject)

option(JRTC_USE_NATIVE "Do use -march=native compiler flag (disable to increase binary portability)" ON)

option(CLANG_FORMAT_CHECK "Enable clang-format check" OFF)
option(CPP_CHECK "Enable cppcheck" OFF)

# Treat all warnings as errors except those explicitly called out in CPP using #warning
SET(CMAKE_C_WARNINGS "-Wall -Werror")

if(JRTC_USE_NATIVE)
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -march=native ${CMAKE_C_WARNINGS}")
else(JRTC_USE_NATIVE)
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 ${CMAKE_C_WARNINGS}")
endif(JRTC_USE_NATIVE)

Message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
if(CMAKE_BUILD_TYPE MATCHES AddressSanitizer)
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -fsanitize=address -O1 -g -lasan")
  add_definitions(-DASAN)
  add_compile_options(-fsanitize=address)
  add_link_options(-fsanitize=address)
elseif(CMAKE_BUILD_TYPE MATCHES Debug)
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O1")
  add_compile_options(--coverage -g)
  add_link_options(--coverage)
else()  # release
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")
endif()

add_definitions(-DPB_FIELD_32BIT)
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake_modules)

# Execute the setup script first
option(INITIALIZE_SUBMODULES "Initialize submodules" ON)
if(INITIALIZE_SUBMODULES)
  execute_process(
      COMMAND bash ./init_submodules.sh
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      RESULT_VARIABLE result
      OUTPUT_VARIABLE output
      ERROR_VARIABLE error
  )
  # Check if the initialization script ran successfully
  if(NOT result EQUAL 0)
      message(FATAL_ERROR "Setup script failed with error: ${error}")
  endif()
endif()

set(JRTC_SOURCE_CODE ${PROJECT_SOURCE_DIR}/src)
set(OUTPUT_DIR ${CMAKE_SOURCE_DIR}/out)
set(ENV{JRTC_OUT_DIR} ${OUTPUT_DIR})
set(JBPF_IO_HEADER_FILES ${OUTPUT_DIR}/inc)

file(MAKE_DIRECTORY ${OUTPUT_DIR})
file(MAKE_DIRECTORY ${OUTPUT_DIR}/inc)

set(JRTC_ROUTER_HEADER_FILES 
  ${CMAKE_SOURCE_DIR}/src/router 
  ${CMAKE_SOURCE_DIR}/src/logger 
  ${CMAKE_SOURCE_DIR}/src/stream_id
  ${CMAKE_SOURCE_DIR}/src/rest_api_lib
  ${CMAKE_SOURCE_DIR}/src/controller
  ${CMAKE_SOURCE_DIR}/src/agent  
)

################ clang-format ################
# Define the clang-format executable
find_program(CLANG_FORMAT_BIN clang-format)

if (NOT CLANG_FORMAT_BIN)
  message(STATUS "clang-format executable not found thus skipping clang-format check")
endif()

# Function to create a clang-format check for a given target
function(add_clang_format_check TARGET_NAME TARGET_SOURCES)
  if (NOT CLANG_FORMAT_CHECK)
    return()
  endif()

  # Ensure clang-format binary is available
  if (CLANG_FORMAT_BIN)
    # Convert all relative paths in TARGET_SOURCES to absolute paths
    set(ABSOLUTE_SOURCES "")
    foreach(SOURCE ${TARGET_SOURCES})
      get_filename_component(ABSOLUTE_SOURCE ${SOURCE} ABSOLUTE)
      list(APPEND ABSOLUTE_SOURCES ${ABSOLUTE_SOURCE})
    endforeach()

    # Add a custom target for clang-format check
    add_custom_target(${TARGET_NAME}_clang_format_check
      COMMAND ${CMAKE_COMMAND} -E echo "Running clang-format check for ${TARGET_NAME}..."
      COMMAND ${CMAKE_SOURCE_DIR}/helper_build_files/clang-format-check.sh ${ABSOLUTE_SOURCES}
      COMMENT "Checking source code formatting with clang-format for ${TARGET_NAME}"
    )

    # Add a custom target to ensure checks are run before build
    add_custom_target(pre_build_checks_${TARGET_NAME}
      DEPENDS ${TARGET_NAME}_clang_format_check
    )

    # Add pre-build checks dependency to the target
    add_dependencies(${TARGET_NAME} pre_build_checks_${TARGET_NAME})
  endif()
endfunction()

################ cppcheck ################
# Function to add cppcheck for a given target
function(add_cppcheck TARGET_NAME TARGET_SOURCES)
  if (NOT CPP_CHECK)
    return()
  endif()

  # Convert all relative paths in TARGET_SOURCES to absolute paths
  set(ABSOLUTE_SOURCES "")
  foreach(SOURCE ${TARGET_SOURCES})
    get_filename_component(ABSOLUTE_SOURCE ${SOURCE} ABSOLUTE)
    list(APPEND ABSOLUTE_SOURCES ${ABSOLUTE_SOURCE})
  endforeach()

  add_custom_target(${TARGET_NAME}_cppcheck
    COMMAND ${CMAKE_COMMAND} -E echo "Running cppcheck for ${TARGET_NAME}..."
    COMMAND ${CMAKE_SOURCE_DIR}/helper_build_files/cppcheck.sh ${ABSOLUTE_SOURCES}
    COMMENT "Checking source code with cppcheck for ${TARGET_NAME}"
  )
  add_custom_target(pre_build_checks_cppcheck_${TARGET_NAME}
    DEPENDS ${TARGET_NAME}_cppcheck
  )
  add_dependencies(${TARGET_NAME} pre_build_checks_cppcheck_${TARGET_NAME})
endfunction()


################ Doxygen ################

find_package(Doxygen)
if (DOXYGEN_FOUND)
  set(DOXYGEN_EXTRACT_STATIC YES)
  set(DOXYGEN_EXTRACT_ALL YES)
  set(DOXYGEN_HIDE_UNDOC_MEMBERS YES)
  set(DOXYGEN_EXTRACT_PRIVATE NO)
  set(DOXYGEN_EXPAND_AS_DEFINED YES)
  set(DOXYGEN_MACRO_EXPANSION YES)
  set(DOXYGEN_HAVE_DOT NO)

  set(DOXYGEN_PROJECT_NAME jrtc)
  set(JRTC_DOXYGEN_IN ${PROJECT_SOURCE_DIR}/README.md
                          ${JRTC_SOURCE_CODE}/agent 
                          ${JRTC_SOURCE_CODE}/controller
                          ${JRTC_SOURCE_CODE}/logger
                          ${JRTC_SOURCE_CODE}/rest_api_lib
                          ${JRTC_SOURCE_CODE}/router
                          ${JRTC_SOURCE_CODE}/stream_id
  )
  set(DOXYGEN_USE_MDFILE_AS_MAINPAGE ${PROJECT_SOURCE_DIR}/README.md)
  set(DOXYGEN_OUTPUT_DIRECTORY jrtc_doc)
  set(DOXYGEN_INCLUDE_PATH ${JRTC_SOURCE_CODE}/agent 
                          ${JRTC_SOURCE_CODE}/controller
                          ${JRTC_SOURCE_CODE}/logger
                          ${JRTC_SOURCE_CODE}/rest_api_lib
                          ${JRTC_SOURCE_CODE}/router
                          ${JRTC_SOURCE_CODE}/stream_id)
  doxygen_add_docs(jrtc_doc ${JRTC_DOXYGEN_IN})

  add_custom_command(TARGET jrtc_doc POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/jrtc_doc ${OUTPUT_DIR}/docs

    ## the front page of the documentation links to the architecture diagram
    COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR}/docs/html/docs
  )
else()
  message("Doxygen not found. Please install doxygen package to generate documentation")
endif()

################ Code coverage ################
enable_testing()

# Code coverage will be checked only in Debug mode
if(CMAKE_BUILD_TYPE MATCHES Debug)
  include(CodeCoverage)
  append_coverage_compiler_flags()

  # Print out the compiler flags to verify
  message(STATUS "Coverage CXX Flags: ${CMAKE_CXX_FLAGS}")
  message(STATUS "Coverage C Flags: ${CMAKE_C_FLAGS}")

else()
  message(STATUS "Not in Debug mode, coverage not enabled")
endif()

# Add the tests directory
add_subdirectory(jrtc_tests)

if(CMAKE_BUILD_TYPE MATCHES Debug)
  message(STATUS "JRTC_TESTS = ${JRTC_TESTS}")

  set(GCOVR_EXCLUDES
    "--exclude=.*/jrtc_tests/.*"
    "--exclude=.*/build/.*"
    "--exclude=.*/out/.*"
    "--exclude=.*jbpf-protobuf.*"
    "--exclude=.*libck-jbpf.*"
  )
  set(GCOVR_INCLUDES
    "--include=.*src/router.*"
    "--include=.*src/logger.*"
    "--include=.*src/stream_id.*"
    "--include=.*src/rest_api_lib.*"
    "--include=.*src/controller.*"
    "--include=.*src/agent.*"
  )

  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lgcov")  

  function(setup_target_for_coverage_gcovr_html NAME EXECUTABLE BASE_DIRECTORY DEPENDENCIES)
    message(STATUS "Setting up ${NAME}_html for coverage gcovr html")
    add_custom_target(${NAME}_html
      COMMAND ${EXECUTABLE}
      COMMAND gcovr ${GCOVR_EXCLUDES}
        --gcov-ignore-parse-errors=negative_hits.warn --html --html-details -o ${NAME}.html --root=${BASE_DIRECTORY}
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      DEPENDS ${DEPENDENCIES}
      COMMENT "Generating HTML coverage report with gcovr"
  )
  endfunction()

  function(setup_target_for_coverage_gcovr_xml NAME EXECUTABLE BASE_DIRECTORY DEPENDENCIES)
      message(STATUS "Setting up ${NAME}_xml for coverage gcovr xml")
      add_custom_target(${NAME}_xml
        COMMAND ${EXECUTABLE}
        COMMAND gcovr ${GCOVR_EXCLUDES}
          --gcov-ignore-parse-errors=negative_hits.warn --xml -o ${NAME}.xml --root=${BASE_DIRECTORY}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS ${DEPENDENCIES}
        COMMENT "Generating XML coverage report with gcovr"
    )
  endfunction()

  setup_target_for_coverage_gcovr_html(
    jrtc_coverage
    ctest
    ${PROJECT_SOURCE_DIR}
    ${JRTC_TESTS}
  )
  setup_target_for_coverage_gcovr_xml(
    jrtc_coverage
    ctest
    ${PROJECT_SOURCE_DIR}
    ${JRTC_TESTS}
  ) 
endif()

# Override add_test and set_tests_properties to do nothing
macro(add_test)
    # Do nothing
endmacro()

macro(set_tests_properties)
    # Do nothing
endmacro()

set(BUILD_TESTING OFF CACHE BOOL "Disable testing" FORCE)
add_subdirectory(jbpf-protobuf)

# Restore the original commands by undefining the macros
unset(add_test)
unset(set_tests_properties)

add_subdirectory(src/logger)
add_subdirectory(src/stream_id)
add_subdirectory(src/router)
add_subdirectory(src/rest_api_lib)
add_subdirectory(src/controller)
add_subdirectory(src/agent)
add_subdirectory(tools/jrtc-ctl)
add_subdirectory(src/pythonapp_loader)
