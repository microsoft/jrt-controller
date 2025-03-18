#!/bin/bash
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.

CURRENT_DIR=$( cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P )
source $CURRENT_DIR/../../setup_jrtc_env.sh

cd $JRTC_PATH
## Testing relative path to deployment.yaml
$JRTC_OUT_DIR/bin/jrtc-ctl load -c ./sample_apps/two_python/deployment.yaml --log-level debug
$JRTC_OUT_DIR/bin/jrtc-ctl load -c ./sample_apps/two_python/deployment2.yaml --log-level debug
