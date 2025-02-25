FROM mcr.microsoft.com/mirror/docker/library/ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
SHELL ["/bin/bash", "-c"]

RUN echo "*** Installing packages"
RUN apt update --fix-missing
RUN apt install -y cmake build-essential libboost-dev git libboost-program-options-dev \
    gcovr doxygen libboost-filesystem-dev libasan6 python3

RUN apt install -y clang-format cppcheck
RUN apt install -y clang gcc-multilib
RUN apt install -y libyaml-cpp-dev
RUN apt install -y libasan6

RUN apt -y install protobuf-compiler python3-pip curl
RUN apt -y install golang-1.23
ENV PATH="$PATH:/root/go/bin:/usr/local/go/bin:/usr/lib/go-1.23/bin"
RUN go env -w GOFLAGS=-buildvcs=false
RUN go install github.com/golangci/golangci-lint/cmd/golangci-lint@v1.60.3
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

RUN pip3 install ctypesgen

RUN apt install -y python3-dev zip

# install rust
RUN apt install -y cargo
ENV PATH="/root/.cargo/bin:${PATH}"

ENTRYPOINT [ "/jrtc/helper_build_files/build_jrtc.sh" ]
