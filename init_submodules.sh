# Copyright (c) Microsoft Corporation. All rights reserved.
rm -rf jbpf-protobuf
rm -rf tools/jrtc-ctl/3p/googleapis

#git submodule update --init --recursive
git submodule update --init

pushd .
cd jbpf-protobuf
./init_submodules.sh
popd


