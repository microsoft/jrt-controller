#!/bin/bash
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.

set -e
if [ -z "$JRTC_PATH" ]; then
    echo "Error: JRTC_PATH environment variable is not set. Please set it before running this script."
    exit 1
fi

rebuild=false
if [ "$2" == "force" ]; then
    rebuild=true
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
else
    echo "...............Skipping jrtc build..............."
fi

echo "...............Building jbpf codelets..............."
cd $JRTC_PATH/$1
source ../../setup_jrtc_env.sh
## Check if Makefile exists
if [ -f "Makefile" ]; then
    make
fi

if [[ $rebuild == true || ! -f $JRTC_PATH/$1/simple_agent_ipc/simple_agent_ipc ]]; then
    echo "...............Building jbpf IPC agent..............."
    cd $JRTC_PATH/$1
    source ../../setup_jrtc_env.sh
    make -C simple_agent_ipc
else
    echo "...............Skipping jbpf IPC agent build..............."
fi
