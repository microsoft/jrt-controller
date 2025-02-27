#!/bin/bash
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.
CURRENT_DIR=$( cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P )
source $CURRENT_DIR/../../setup_jrtc_env.sh

JRTC_IMAGE=jrtc.azurecr.io/jrtc-mariner
sudo docker run --rm -it --privileged --network host \
    --cap-add IPC_LOCK  --cap-add SYS_NICE   --cap-add=SYS_ADMIN --shm-size=1g \
    --mount type=tmpfs,destination=/dev/shm  -v /dev/hugepages:/dev/hugepages   \
    -e VERBOSE=1 -e JRTC_PATH=/jrtc -e JRTC_APP_PATH=/jrtc/out/lib/ \
    -v /tmp/jrtc/out:/jrtc/out -v /tmp/jbpf:/tmp/jbpf --entrypoint=/bin/bash $JRTC_IMAGE


