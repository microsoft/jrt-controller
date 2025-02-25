# Copyright (c) Microsoft Corporation. All rights reserved.
#!/bin/bash

export JRTC_PATH="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd)"
export JRTC_OUT_DIR=$JRTC_PATH/out
export JRTC_APP_PATH=$JRTC_OUT_DIR/lib/
export JRTC_PYTHON_INC=$JRTC_PATH/src/wrapper_apis
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$JRTC_OUT_DIR/lib
export PYTHONMALLOC=malloc
source $JRTC_PATH/jbpf-protobuf/setup_jbpfp_env.sh
