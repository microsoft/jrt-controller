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

if [[ $rebuild == true || ! -f $JRTC_PATH/sample_apps/$1/jbpf_codelets/data_generator/data_generator.o || ! -f $JRTC_PATH/sample_apps/$1/jbpf_codelets/simple_input/simple_input.o  || ! -f $JRTC_PATH/sample_apps/$1/jbpf_codelets/data_generator_protobuf/data_generator_pb_codelet.o ]]; then
    echo "...............Building jbpf codelets..............."
    cd $JRTC_PATH/sample_apps/$1
    source ../../setup_jrtc_env.sh
    make -C jbpf_codelets/data_generator
    make -C jbpf_codelets/simple_input
    make -C jbpf_codelets/data_generator_protobuf
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

