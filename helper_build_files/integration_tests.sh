#!/bin/bash
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.

set -ex
if [ -z "$JRTC_PATH" ]; then
    echo "Error: JRTC_PATH environment variable is not set. Please set it before running this script."
    exit 1
fi

echo "...............Building all..............."
## first parameter is the app name
## second parameter: specify "force" to force build jrt-controller
time $JRTC_PATH/helper_build_files/build_all.sh $1 $2

echo "...............Running JRTC Tests..............."
time $JRTC_PATH/$1/run.sh
