#!/bin/bash
set -e
if [ -z "$JRTC_PATH" ]; then
    echo "Error: JRTC_PATH environment variable is not set. Please set it before running this script."
    exit 1
fi

rebuild=false
if [ "$2" == "force" ]; then
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

if [[ $rebuild == true || ! -f $JRTC_PATH/sample_apps/$1/jbpf_codelets/data_generator/data_generator.o || ! -f $JRTC_PATH/sample_apps/$1/jbpf_codelets/simple_input/simple_input.o  ]]; then
    echo "...............Building jbpf codelets..............."
    cd $JRTC_PATH/sample_apps/$1
    source ../../setup_jrtc_env.sh
    make -C jbpf_codelets/data_generator
    make -C jbpf_codelets/simple_input
    ## Check if Makefile exists
    if [ -f "Makefile" ]; then
        make
    fi
else
    echo "...............Skipping jbpf codelets build..............."
fi

if [[ $rebuild == true || ! -f $JRTC_PATH/sample_apps/$1/simple_agent_ipc/simple_agent_ipc ]]; then
    echo "...............Building jbpf IPC agent..............."
    cd $JRTC_PATH/sample_apps/$1
    source ../../setup_jrtc_env.sh
    make -C simple_agent_ipc
else
    echo "...............Skipping jbpf IPC agent build..............."
fi

