# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.

FROM mcr.microsoft.com/azurelinux/base/core:3.0
RUN echo "*** Installing packages"
RUN tdnf upgrade tdnf --refresh -y
RUN tdnf -y update
RUN tdnf -y install build-essential cmake golang git which awk jq
RUN tdnf -y install yaml-cpp-devel yaml-cpp-static boost-devel python3-devel gcovr
RUN tdnf -y install python3-pip python3-setuptools userspace-rcu-devel
RUN tdnf -y install meson ninja-build wget tar libcmocka libcmocka-devel libcmocka-static doxygen
RUN tdnf -y install rust
RUN pip3 install pyelftools
#RUN sed -i 's/-targets.cmake/yaml-cpp-targets.cmake/g' /usr/share/cmake/yaml-cpp/yaml-cpp-config.cmake
RUN pip3 install python3-protobuf protobuf==3.19.3 ctypesgen dataclasses crc numpy pyyaml requests regex

