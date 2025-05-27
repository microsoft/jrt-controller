#!/bin/bash

# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.

CURRENT_DIR=$( cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P )
source $CURRENT_DIR/../../setup_jrtc_env.sh

echo "Unloading App1"
$JRTC_OUT_DIR/bin/jrtc-ctl unload -c advanced_example1.yaml --log-level debug

echo "Unloading App2"
$JRTC_OUT_DIR/bin/jrtc-ctl unload -c advanced_example2.yaml --log-level debug
