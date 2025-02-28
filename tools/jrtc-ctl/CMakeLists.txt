# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.

cmake_minimum_required(VERSION 3.16)
project(jrtc-ctl)

# Set the Go executable
find_program(GO_EXECUTABLE go)

if(NOT GO_EXECUTABLE)
    message(FATAL_ERROR "Go compiler not found. Please install Go.")
endif()

# Set variables
set(BINARY_NAME jrtc-ctl)
set(JRTCCTL_CGO_CFLAGS "-I${OUTPUT_DIR}/inc")
set(JRTCCTL_CGO_LDFLAGS "-L${OUTPUT_DIR}/lib -ljrtc_router_stream_id_static")

MESSAGE(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

if(CMAKE_BUILD_TYPE MATCHES AddressSanitizer)
    set(JRTCCTL_CGO_CFLAGS "${JRTCCTL_CGO_CFLAGS} -g -O1 -fsanitize=address -lasan")
    set(JRTCCTL_CGO_LDFLAGS "${JRTCCTL_CGO_LDFLAGS} -g -O1 -fsanitize=address -lasan")
elseif(CMAKE_BUILD_TYPE MATCHES Debug)
    # coverage
    set(JRTCCTL_CGO_CFLAGS "${JRTCCTL_CGO_CFLAGS} -g -O1 -fprofile-arcs -ftest-coverage --coverage")
    set(JRTCCTL_CGO_LDFLAGS "${JRTCCTL_CGO_LDFLAGS} -g -O1 -fprofile-arcs -ftest-coverage  --coverage -lgcov")
    add_compile_options(--coverage -g)
    add_link_options(--coverage)
else()
    set(JRTCCTL_CGO_CFLAGS "${JRTCCTL_CGO_CFLAGS} -O3")
    set(JRTCCTL_CGO_LDFLAGS "${JRTCCTL_CGO_LDFLAGS} -O3")
endif()

set(JRTCCTL_TEST_TAGS "ci")
option(JRTCCTL_REGENERATE_SNAPSHOT false)

# Add a custom target to build the Go application
add_custom_target(${BINARY_NAME} ALL
    COMMAND ${GO_EXECUTABLE} mod tidy || (echo "Error: 'go mod tidy' failed" && exit 1)
    COMMAND echo "Installing protoc-gen-go and protoc-gen-go-grpc"
    COMMAND ${GO_EXECUTABLE} install
            github.com/grpc-ecosystem/grpc-gateway/v2/protoc-gen-grpc-gateway
            github.com/grpc-ecosystem/grpc-gateway/v2/protoc-gen-openapiv2
            google.golang.org/protobuf/cmd/protoc-gen-go
            google.golang.org/grpc/cmd/protoc-gen-go-grpc
    COMMAND echo "Generating protobuf files"
    COMMAND ${GO_EXECUTABLE} generate ./... || (echo "Error: 'go generate' failed" && exit 1)
    COMMAND echo "Building jrtc-ctl application"
    COMMAND ${CMAKE_COMMAND} -E env
        CGO_CFLAGS=${JRTCCTL_CGO_CFLAGS}
        CGO_LDFLAGS=${JRTCCTL_CGO_LDFLAGS}
        ${GO_EXECUTABLE} build --trimpath -o ${OUTPUT_DIR}/bin/${BINARY_NAME} main.go || (echo "Error: 'go build' failed" && exit 1)
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building jrtc-ctl application"
)

# Add a custom target to run linting
find_program(GOLANGCI_LINT_EXECUTABLE golangci-lint)

if(NOT GOLANGCI_LINT_EXECUTABLE)
    message(WARNING "golangci-lint not found. Skipping linting step.")
else()
    add_custom_target(${BINARY_NAME}-lint ALL
        COMMAND ${CMAKE_COMMAND} -E env
            CGO_CFLAGS=${JRTCCTL_CGO_CFLAGS}
            CGO_LDFLAGS=${JRTCCTL_CGO_LDFLAGS}
            ${GOLANGCI_LINT_EXECUTABLE} run --concurrency=1
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Running linting for jrtc-ctl"
    )
endif()

# Add a custom target to run tests
add_custom_target(${BINARY_NAME}-test ALL
    COMMAND ${CMAKE_COMMAND} -E env
        CGO_CFLAGS=${JRTCCTL_CGO_CFLAGS}
        CGO_LDFLAGS=${JRTCCTL_CGO_LDFLAGS}
        ${GO_EXECUTABLE} test -v ./... -tags=${JRTCCTL_TEST_TAGS}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running tests for jrtc-ctl"
)

## TODO: Add it to `make test`
#add_dependencies(test ${BINARY_NAME}-test)
#add_dependencies(lint ${BINARY_NAME}-lint)