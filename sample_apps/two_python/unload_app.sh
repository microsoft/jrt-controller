#!/bin/bash

# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.


CURRENT_DIR=$( cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P )
source $CURRENT_DIR/../../setup_jrtc_env.sh

cd $JRTC_PATH
## Testing full path to deployment.yaml
$JRTC_OUT_DIR/bin/jrtc-ctl unload -c ${CURRENT_DIR}/deployment2.yaml --log-level trace
$JRTC_OUT_DIR/bin/jrtc-ctl unload -c ${CURRENT_DIR}/deployment.yaml --log-level trace
