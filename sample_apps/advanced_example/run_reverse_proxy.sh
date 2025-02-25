#!/bin/bash
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.
CURRENT_DIR=$( cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P )

source $CURRENT_DIR/../../setup_jrtc_env.sh
$JBPF_PATH/examples/reverse_proxy/proxy --address "/tmp/jbpf/jbpf_lcm_ipc"
