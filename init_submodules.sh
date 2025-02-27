#!/bin/bash
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.
rm -rf jbpf-protobuf
rm -rf tools/jrtc-ctl/3p/googleapis

#git submodule update --init --recursive
git submodule update --init

pushd .
cd jbpf-protobuf
./init_submodules.sh
popd


