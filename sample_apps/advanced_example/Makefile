# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.

SRC1 = advanced_example1.c
SRC2 = advanced_example2.c
LIBRARY1 = app1
LIBRARY2 = app2

# Define the target rules
all: build_dirs $(LIBRARY1) $(LIBRARY2)

# Define header paths
INCLUDE_DIRS = -I$(JRTC_OUT_DIR)/inc -I$(NANO_PB) -I../jbpf_codelets/data_generator -I../jbpf_codelets/data_generator_protobuf -I../jbpf_codelets/simple_input_protobuf

# Define compiler flags
CFLAGS = -Wall -fPIC $(INCLUDE_DIRS) -fno-gnu-unique -g -DPB_FIELD_32BIT=1

# Build dependencies first
build_dirs:
	for dir in ../jbpf_codelets/*; do \
			make -C $$dir; \
	done; \
	make -C simple_agent_ipc;

# Rule to compile the C source code
$(LIBRARY1):
	gcc -shared -o $(LIBRARY1).so $(SRC1) $(CFLAGS)

$(LIBRARY2):
	gcc -shared -o $(LIBRARY2).so $(SRC2) $(CFLAGS)

# Clean rule to remove generated files and the shared libraries
clean:
	for dir in ../jbpf_codelets/*; do \
			make -C $$dir clean; \
	done; \
	make -C simple_agent_ipc clean; \
	rm -f $(LIBRARY1).so $(LIBRARY2).so

# .PHONY is used to declare the targets that are not files
.PHONY: all clean build_dirs
