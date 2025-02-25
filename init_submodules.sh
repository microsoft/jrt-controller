# Copyright (c) Microsoft Corporation. All rights reserved.
rm -rf jbpf-protobuf

#git submodule update --init --recursive
git submodule update --init

pushd .
cd jbpf-protobuf
./init_submodules.sh
popd


