# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.
FROM mcr.microsoft.com/azurelinux/base/core:3.0

RUN echo "*** Installing packages"
RUN tdnf upgrade tdnf --refresh -y
RUN tdnf -y update
RUN tdnf -y install build-essential cmake git
RUN tdnf -y install yaml-cpp-devel yaml-cpp-static boost-devel gcovr clang python3
RUN tdnf -y install doxygen

## clang-format
RUN tdnf -y install clang-tools-extra

RUN tdnf -y install golang ca-certificates jq protobuf python3-pip curl
ENV PATH="$PATH:/root/go/bin"
RUN go env -w GOFLAGS=-buildvcs=false
RUN go install github.com/golangci/golangci-lint/cmd/golangci-lint@v1.64.5
ENV PATH="/root/go/bin:${PATH}"

RUN echo "Installing protoc-gen-go and protoc-gen-go-grpc" && \
    go install github.com/grpc-ecosystem/grpc-gateway/v2/protoc-gen-grpc-gateway@latest && \
    go install github.com/grpc-ecosystem/grpc-gateway/v2/protoc-gen-openapiv2@latest && \
    go install google.golang.org/protobuf/cmd/protoc-gen-go@latest && \
    go install google.golang.org/grpc/cmd/protoc-gen-go-grpc@latest

WORKDIR /jrtc
COPY . /jrtc

RUN tdnf install -y python3-devel rust protobuf-devel

RUN pip3 install -r /jrtc/jbpf-protobuf/3p/nanopb/requirements.txt
## TODO: We prefer the following rather than above
## But the protobuf 3.20 is a hard requirement which is not available on ubuntu 22.04 through apt.
## RUN apt install -y python3-protobuf python3-grpcio

## install pgrep
RUN tdnf install -y procps-ng

RUN pip3 install ctypesgen

ENTRYPOINT [ "/jrtc/helper_build_files/build_jrtc.sh" ]
