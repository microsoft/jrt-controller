# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.

# Define the target rules
all: build_dirs

# Build dependencies first
build_dirs:
	for dir in ../jbpf_codelets/*; do \
			make -C $$dir; \
	done; \
	make -C simple_agent_ipc;

# Clean rule to remove generated files and the shared libraries
clean:
	for dir in ../jbpf_codelets/*; do \
			make -C $$dir clean; \
	done; \
	make -C simple_agent_ipc clean; \

# .PHONY is used to declare the targets that are not files
.PHONY: all clean build_dirs
