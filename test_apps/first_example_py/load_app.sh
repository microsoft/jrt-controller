#!/bin/bash

# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.

CURRENT_DIR=$( cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P )
source $CURRENT_DIR/../../setup_jrtc_env.sh

cd $JRTC_PATH
## Testing relative path to deployment.yaml
$JRTC_OUT_DIR/bin/jrtc-ctl load -c ./test_apps/first_example_py/deployment.yaml --log-level trace


## second time loading the same app should fail
$JRTC_OUT_DIR/bin/jrtc-ctl load -c ./test_apps/first_example_py/deployment.yaml --log-level trace
if [ $? -ne 0 ]; then
    echo "App is already loaded, as expected."
else
    echo "App should not be loaded again, but it was."
    exit 1
fi
