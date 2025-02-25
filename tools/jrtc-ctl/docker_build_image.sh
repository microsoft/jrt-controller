#!/bin/bash

CURRENT_DIR=$( cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P )

IMAGE_NAME=jrtc.azurecr.io/jrtc-ctl
JRTC_IMAGE=jrtc.azurecr.io/jrtc-mariner

IMAGE_TAG=latest
JRTC_IMAGE_TAG=latest
SHOULD_PUSH=false
LABEL_ARGS=""
docker_cmd="sudo docker"

while getopts "t:d:j:pu" option; do
    case $option in
        t)
            IMAGE_TAG="$OPTARG";;
        d)
            JRTC_IMAGE_TAG="$OPTARG";;
        j)
            JBPF_BUILDER_IMAGE_TAG="$OPTARG";;
        p)
            SHOULD_PUSH=true;;
        u) # Unpriviledged
            docker_cmd="docker";;
        \?) # Invalid option
            echo "Error: Invalid option"
            exit 1;;
    esac
done

pushd .

cd $JRTC_PATH

$docker_cmd build \
    -f $CURRENT_DIR/deploy/Dockerfile \
    --build-arg JRTC_IMAGE=$JRTC_IMAGE \
    --build-arg JRTC_IMAGE_TAG=$JRTC_IMAGE_TAG \
    $LABEL_ARGS \
    -t $IMAGE_NAME:$IMAGE_TAG $(dirname $(dirname $CURRENT_DIR))

popd

if $SHOULD_PUSH; then
    $docker_cmd push $IMAGE_NAME:$IMAGE_TAG
fi

