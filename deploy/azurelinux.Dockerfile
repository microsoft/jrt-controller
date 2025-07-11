# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.
FROM mcr.microsoft.com/azurelinux/base/core:3.0

LABEL org.opencontainers.image.source="https://github.com/microsoft/jrt-controller"
LABEL org.opencontainers.image.authors="Microsoft Corporation"
LABEL org.opencontainers.image.licenses="MIT"
LABEL org.opencontainers.image.description="jrt-controller for Azure Linux"

RUN echo "*** Installing packages"
RUN tdnf upgrade tdnf --refresh -y
RUN tdnf -y update
RUN tdnf -y install build-essential cmake git
RUN tdnf -y install yaml-cpp-devel yaml-cpp-static boost-devel gcovr clang python3
RUN tdnf -y install doxygen
RUN tdnf -y install libyaml-devel

## clang-format
RUN tdnf -y install clang-tools-extra

RUN tdnf -y install golang ca-certificates jq protobuf python3-pip curl
ENV PATH="$PATH:/root/go/bin"
RUN go env -w GOFLAGS=-buildvcs=false
RUN go install github.com/golangci/golangci-lint/cmd/golangci-lint@v1.64.5
ENV PATH="/root/go/bin:${PATH}"

RUN echo "Installing specific versions of protoc-gen-go and protoc-gen-go-grpc" && \
    go install github.com/grpc-ecosystem/grpc-gateway/v2/protoc-gen-grpc-gateway@v2.26.1 && \
    go install github.com/grpc-ecosystem/grpc-gateway/v2/protoc-gen-openapiv2@v2.26.1 && \
    go install google.golang.org/protobuf/cmd/protoc-gen-go@v1.36.5 && \
    go install google.golang.org/grpc/cmd/protoc-gen-go-grpc@v1.5.1

WORKDIR /jrtc
COPY . /jrtc

RUN tdnf install -y python3-devel rust protobuf-devel

RUN pip3 install -r /jrtc/jbpf-protobuf/3p/nanopb/requirements.txt
## TODO: We prefer the following rather than above
## But the protobuf 3.20 is a hard requirement which is not available on ubuntu 22.04 through apt.
## RUN apt install -y python3-protobuf python3-grpcio

## install pgrep
RUN tdnf install -y procps-ng

RUN pip3 install ctypesgen requests 


## build the jrtc and doxygen
RUN DOXYGEN=1 /jrtc/helper_build_files/build_jrtc.sh

## check if /jrtc/out/bin/jrtc exists
RUN if [ ! -f /jrtc/out/bin/jrtc ]; then echo "build error: jrtc not found"; exit 1; fi

ENTRYPOINT [ "/jrtc/helper_build_files/build_jrtc.sh" ]
