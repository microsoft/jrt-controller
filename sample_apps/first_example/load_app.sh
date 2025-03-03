#!/bin/bash

# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.


CURRENT_DIR=$( cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P )
source $CURRENT_DIR/../../setup_jrtc_env.sh

$JRTC_OUT_DIR/bin/jrtc-ctl load -c deployment.yaml --log-level trace
