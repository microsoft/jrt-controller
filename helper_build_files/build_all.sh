#!/bin/bash
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.
set -e
if [ -z "$JRTC_PATH" ]; then
    echo "Error: JRTC_PATH environment variable is not set. Please set it before running this script."
    exit 1
fi

rebuild=false
if [ "$1" == "force" ]; then
    rebuild=true
fi

if [[ $rebuild == true || ! -d $JRTC_PATH/jbpf-protobuf/jbpf/out ]]; then
    echo "...............Building jbpf..............."
    cd $JRTC_PATH/jbpf-protobuf/jbpf
    ./init_and_patch_submodules.sh
    source setup_jbpf_env.sh
    mkdir -p build
    cd build
    cmake ../
    make -j
else
    echo "...............Skipping jbpf build..............."
fi

if [[ $rebuild == true || ! -d $JRTC_PATH/out ]]; then
    echo "...............Building jrtc..............."
    cd $JRTC_PATH
    ./init_submodules.sh
    source setup_jrtc_env.sh
    mkdir -p build
    cd build
    cmake ../
    make -j
    if [[ ! -f $JRTC_PATH/out/lib/libpython_loader.so || -f $JRTC_PATH/out/lib/jrtc_bindings.py ]]; then
        echo "...............Building Python Loader..............."
        make jrtc_pythonapp_loader
    else
        echo "...............Skipping Python Loader..............."
    fi
else
    echo "...............Skipping jrtc build..............."
fi
