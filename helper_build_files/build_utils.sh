#!/bin/bash
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.
## provide bash functions to help build docker containers

## get cmake build flags
### env parameter: JRTC_DEBUG
### env parameter: SANITIZER
### env parameter: CLANG_FORMAT_CHECK
### env parameter: CPP_CHECK
### evn parameter: BUILD_PYTHON_LOADER
### env parameter: INITIALIZE_SUBMODULES
get_flags() {
    FLAGS=""
    OUTPUT=""
    if [[ "$JRTC_DEBUG" == "1" ]]; then
        OUTPUT="$OUTPUT Building JRTC with debug symbols\n"
        FLAGS="$FLAGS -DCMAKE_BUILD_TYPE=Debug"
    fi
    if [[ "$SANITIZER" == "1" ]]; then
        OUTPUT="$OUTPUT Building with Address Sanitizer\n"
        FLAGS="$FLAGS -DUSE_NATIVE=OFF -DCMAKE_BUILD_TYPE=AddressSanitizer"
    fi
    if [[ "$CLANG_FORMAT_CHECK" == "1" ]]; then
        OUTPUT="$OUTPUT Checking with clang-format\n"
        FLAGS="$FLAGS -DCLANG_FORMAT_CHECK=on"
    else 
        OUTPUT="$OUTPUT Skipping clang-format check\n"
    fi
    if [[ "$CPP_CHECK" == "1" ]]; then
        OUTPUT="$OUTPUT Checking with cppcheck\n"
        FLAGS="$FLAGS -DCPP_CHECK=on"
    else
        OUTPUT="$OUTPUT Skipping cppcheck\n"
    fi
    if [[ "$BUILD_PYTHON_LOADER" == "1" || "$BUILD_PYTHON_LOADER" == "" ]]; then
        OUTPUT="$OUTPUT Building Python Loader\n"
        FLAGS="$FLAGS -DBUILD_PYTHON_LOADER=on"
    else
        OUTPUT="$OUTPUT Skipping Python Loader\n"
    fi
    if [[ "$INITIALIZE_SUBMODULES" == "1" || "$INITIALIZE_SUBMODULES" == "" ]]; then
        OUTPUT="$OUTPUT Initialize Submodules\n"
        FLAGS="$FLAGS -DINITIALIZE_SUBMODULES=on"
    else
        OUTPUT="$OUTPUT Skipping INITIALIZE_SUBMODULES\n"
    fi
}
