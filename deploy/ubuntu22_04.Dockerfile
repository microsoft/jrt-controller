# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.
FROM mcr.microsoft.com/mirror/docker/library/ubuntu:22.04

LABEL org.opencontainers.image.source="https://github.com/microsoft/jrt-controller"
LABEL org.opencontainers.image.authors="Microsoft Corporation"
LABEL org.opencontainers.image.licenses="MIT"
LABEL org.opencontainers.image.description="jrt-controller for Ubuntu 22.04"

ENV DEBIAN_FRONTEND=noninteractive
SHELL ["/bin/bash", "-c"]

RUN echo "*** Installing packages" && \
    apt update --fix-missing && \
    apt install -y software-properties-common && \
    add-apt-repository ppa:deadsnakes/ppa && \
    apt update && \
    apt install -y python3.12 python3.12-dev python3.12-venv curl && \
    python3.12 -m ensurepip && \
    python3.12 -m pip install --upgrade pip && \
    ln -sfn /usr/bin/python3.12 /usr/bin/python3 && \
    ln -sfn /usr/local/bin/pip3 /usr/bin/pip3

# Verify Python version
RUN python3 --version && pip3 --version

RUN apt install -y cmake build-essential libboost-dev git libboost-program-options-dev \
    gcovr doxygen libboost-filesystem-dev libasan6 python3

RUN apt install -y clang-format cppcheck
RUN apt install -y clang gcc-multilib
RUN apt install -y libyaml-cpp-dev
RUN apt install -y libasan6
RUN apt install -y libyaml-dev

RUN apt -y install protobuf-compiler python3-pip curl
RUN apt -y install golang-1.23
ENV PATH="$PATH:/root/go/bin:/usr/local/go/bin:/usr/lib/go-1.23/bin"
RUN go env -w GOFLAGS=-buildvcs=false
RUN go install github.com/golangci/golangci-lint/cmd/golangci-lint@v1.64.5
ENV PATH="/root/go/bin:${PATH}"

RUN echo "Installing protoc-gen-go and protoc-gen-go-grpc" && \
    go install github.com/grpc-ecosystem/grpc-gateway/v2/protoc-gen-grpc-gateway@latest && \
    go install github.com/grpc-ecosystem/grpc-gateway/v2/protoc-gen-openapiv2@latest && \
    go install google.golang.org/protobuf/cmd/protoc-gen-go@latest && \
    go install google.golang.org/grpc/cmd/protoc-gen-go-grpc@latest

# Set the working directory and copy the project files
WORKDIR /jrtc
COPY . /jrtc

RUN pip3 install -r /jrtc/jbpf-protobuf/3p/nanopb/requirements.txt
## TODO: We prefer the following rather than above
## But the protobuf 3.20 is a hard requirement which is not available on ubuntu 22.04 through apt.
## RUN apt install -y python3-protobuf python3-grpcio

RUN pip3 install ctypesgen requests

RUN apt install -y python3-dev zip

# install rust
RUN apt install -y cargo
ENV PATH="/root/.cargo/bin:${PATH}"

## build the jrtc and doxygen
RUN DOXYGEN=1 /jrtc/helper_build_files/build_jrtc.sh

## check if /jrtc/out/bin/jrtc exists
RUN if [ ! -f /jrtc/out/bin/jrtc ]; then echo "build error: jrtc not found"; exit 1; fi

ENTRYPOINT [ "/jrtc/helper_build_files/build_jrtc.sh" ]
