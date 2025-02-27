# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.

ARG JRTC_IMAGE
ARG JRTC_IMAGE_TAG

FROM ${JRTC_IMAGE}:${JRTC_IMAGE_TAG} AS builder

# Install clang
RUN tdnf -y install clang

# Install protobuf modules
RUN tdnf -y install protobuf-compiler protobuf-devel

# Update Golang
ENV GO_VERSION=1.23.2
ENV GO_ROOT=/usr/bin/go
RUN rm -rf $GO_ROOT
ENV PATH=$PATH:$GO_ROOT/bin
RUN wget https://golang.org/dl/go$GO_VERSION.linux-amd64.tar.gz && \
    tar -C /usr/bin -xzf go$GO_VERSION.linux-amd64.tar.gz && \
    rm go$GO_VERSION.linux-amd64.tar.gz
RUN go version

# Install golangci-lint
RUN tdnf install -y ca-certificates
RUN curl -sSfL https://raw.githubusercontent.com/golangci/golangci-lint/master/install.sh | sh -s -- -b /root/go/bin v1.60.3
ENV PATH="$PATH:/root/go/bin"

ADD . /jrtc

# Build jrt-controller
ENV USE_NATIVE=OFF
RUN /jrtc/helper_build_files/build_jrtc.sh

# # Set up environment
ENV JRTC_PATH=/jrtc
ENV LD_LIBRARY_PATH="/jrtc/out/lib"

# # Build and run static code analysis for jrtc-ctl
WORKDIR /jrtc/tools/jrtc-ctl
RUN make mod generate
RUN make
RUN make lint test -j

FROM mcr.microsoft.com/azurelinux/base/core:3.0

ENV JRTC_PATH=/jrtc
ENV LD_LIBRARY_PATH="/jrtc/out/lib"

COPY --from=builder /jrtc/out /jrtc/out
COPY --from=builder /jrtc/tools/jrtc-ctl/jrtc-ctl /usr/local/bin/jrtc-ctl
COPY --from=builder /jrtc/tools/schemas/jrtc-ctl.schema.json /schemas/jrtc-ctl.schema.json

ENTRYPOINT ["jrtc-ctl"]
