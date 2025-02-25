#!/bin/bash
# Copyright (c) Microsoft Corporation. All rights reserved.

CURRENT_DIR=$( cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P )
source $CURRENT_DIR/../../setup_jrtc_env.sh

cd $JRTC_PATH
## Testing full path to deployment.yaml
$JRTC_OUT_DIR/bin/jrtc-ctl unload -c ${CURRENT_DIR}/deployment.yaml --log-level trace
