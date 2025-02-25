#!/bin/bash
# Copyright (c) Microsoft Corporation. All rights reserved.

CURRENT_DIR=$( cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P )
source $CURRENT_DIR/../../setup_jrtc_env.sh

$JRTC_PATH/out/bin/jrtc-ctl decoder run --decoder-data-enabled
