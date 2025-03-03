#!/bin/bash

# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.

CURRENT_DIR=$( cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P )

source $CURRENT_DIR/../../setup_jrtc_env.sh
export PYTHONMALLOC=malloc
${JRTC_OUT_DIR}/bin/jrtc
