# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.
FROM mcr.microsoft.com/mirror/docker/library/ubuntu:24.04

LABEL org.opencontainers.image.source="https://github.com/microsoft/jrt-controller"
LABEL org.opencontainers.image.authors="Microsoft Corporation"
LABEL org.opencontainers.image.licenses="MIT"
LABEL org.opencontainers.image.description="jrt-controller for Ubuntu 24.04"

ENV DEBIAN_FRONTEND=noninteractive
SHELL ["/bin/bash", "-c"]
ENV CLANG_FORMAT_CHECK=1
ENV CPP_CHECK=1

RUN echo "*** Installing packages"
RUN apt update --fix-missing
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
RUN go install github.com/golangci/golangci-lint/cmd/golangci-lint@v1.64.5
ENV PATH="/root/go/bin:${PATH}"

RUN apt install -y golang-goprotobuf-dev
RUN go install google.golang.org/protobuf/cmd/protoc-gen-go@latest
RUN go install google.golang.org/grpc/cmd/protoc-gen-go-grpc@latest

# Set the working directory and copy the project files
WORKDIR /jrtc
COPY . /jrtc

RUN apt install -y nanopb

RUN pip3 install ctypesgen --break-system-packages
RUN pip3 install requests --break-system-packages

RUN apt install -y python3-dev zip

RUN pip3 install -r /jrtc/jbpf-protobuf/3p/nanopb/requirements.txt --break-system-packages
## TODO: We prefer the following rather than above
## But the protobuf 3.20 is a hard requirement which is not available on ubuntu 22.04 through apt.
## RUN apt install -y python3-protobuf python3-grpcio

# install rust
RUN apt install -y cargo
ENV PATH="/root/.cargo/bin:${PATH}"

## build the jrtc and doxygen
RUN DOXYGEN=1 /jrtc/helper_build_files/build_jrtc.sh

## check if /jrtc/out/bin/jrtc exists
RUN if [ ! -f /jrtc/out/bin/jrtc ]; then echo "build error: jrtc not found"; exit 1; fi

ENTRYPOINT [ "/jrtc/helper_build_files/build_jrtc.sh" ]
