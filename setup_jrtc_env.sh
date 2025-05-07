#!/bin/bash
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.

export PYTHONMALLOC=malloc
export JRTC_PATH="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd)"
export JRTC_OUT_DIR=$JRTC_PATH/out
export JRTC_APP_PATH=$JRTC_OUT_DIR/lib/
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$JRTC_OUT_DIR/lib
source $JRTC_PATH/jbpf-protobuf/setup_jbpfp_env.sh
