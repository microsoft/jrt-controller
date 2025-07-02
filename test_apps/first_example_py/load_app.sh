#!/bin/bash

# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.

CURRENT_DIR=$( cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P )
source $CURRENT_DIR/../../setup_jrtc_env.sh

cd $JRTC_PATH
## Testing relative path to deployment.yaml
if ! $JRTC_OUT_DIR/bin/jrtc-ctl load -c ./test_apps/first_example_py/deployment.yaml --log-level trace; then
    echo "Failed to load app with relative path to deployment.yaml"
    #exit 1
fi

## second time loading the same app should fail
echo "Loading the same app again should fail as expected."
if ! $JRTC_OUT_DIR/bin/jrtc-ctl load -c ./test_apps/first_example_py/deployment.yaml --log-level trace; then
    echo "App is already loaded, as expected."
    #exit 0
else
    echo "App should not be loaded again, but it was."
    #exit 1
fi
