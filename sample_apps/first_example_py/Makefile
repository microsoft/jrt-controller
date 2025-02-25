# Define the target rules
all: build_dirs 

## Define header paths
INCLUDE_DIRS = -I$(JRTC_OUT_DIR)/inc -I$(NANO_PB) 

## Define compiler flags
CFLAGS = -Wall -fPIC $(INCLUDE_DIRS) -fno-gnu-unique -g -DPB_FIELD_32BIT=1

# Build dependencies first
build_dirs:
	for dir in ./jbpf_codelets/*; do \
			make -C $$dir; \
	done; \
	make -C simple_agent_ipc;


# Clean rule to remove generated files and the shared libraries
clean:
	for dir in ./jbpf_codelets/*; do \
			make -C $$dir clean; \
	done; \
	make -C simple_agent_ipc clean; \

# .PHONY is used to declare the targets that are not files
.PHONY: all clean build_dirs
