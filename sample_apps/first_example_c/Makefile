# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.

SRC = first_example.c
LIBRARY_NAME = app1

# Define the target rules
all: $(LIBRARY_NAME)

# Define header paths
INCLUDE_DIRS = -I$(JRTC_OUT_DIR)/inc -I$(NANO_PB) -I../jbpf_codelets/data_generator -I../jbpf_codelets/simple_input

# Define compiler flags
CFLAGS = -Wall -fPIC $(INCLUDE_DIRS) -fno-gnu-unique -g -DPB_FIELD_32BIT=1

# Link in Add jrtc_app.sh
LD_FLAGS=-L$(JRTC_APP_PATH) -ljrtc_app

# Rule to compile the C source code
$(LIBRARY_NAME):
	for dir in ../jbpf_codelets/*; do \
			make -C $$dir; \
	done; \
	make -C simple_agent_ipc; \
	gcc -shared -o $(LIBRARY_NAME).so $(SRC) $(CFLAGS) $(LD_FLAGS)

# Clean rule to remove generated files and the shared library
clean:
	for dir in ../jbpf_codelets/*; do \
			make -C $$dir clean; \
	done; \
	make -C simple_agent_ipc clean; \
	rm -f $(LIBRARY_NAME).so

# .PHONY is used to declare the targets that are not files
.PHONY: all clean
