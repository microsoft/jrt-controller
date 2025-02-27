# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.
CC = clang
INC= -I${JRTC_OUT_DIR}/inc -I${NANO_PB}
CFLAGS = -O2 -target bpf -Wall -DJBPF_EXPERIMENTAL_FEATURES -DJBPF_DEBUG_ENABLED -D__x86_64__ -DPB_FIELD_32BIT=1 -DUSE_JBPF_PRINTF_HELPER -DJBPF_PRINTF_HELPER_ENABLED -DJBPF_DEBUG
JBPF_VERIFIER = ${JRTC_OUT_DIR}/bin/jbpf_verifier_cli
JBPF_PROTOBUF_CLI_BIN = ${JRTC_OUT_DIR}/bin/jbpf_protobuf_cli
SOURCES = $(wildcard *.c)
OBJECTS = $(SOURCES:.c=.o)
PROTO_SOURCES = $(wildcard *.proto)
PROTO_OBJECTS = $(PROTO_SOURCES:.proto=.pb)

all: generate $(PROTO_OBJECTS) $(OBJECTS)

$(OBJECTS): %.o : %.c
	$(CC) $(INC) $(CFLAGS) -c $<
	$(JBPF_VERIFIER) $@

clean:
	rm -f *.o *.pb.h *.so *.pb

$(PROTO_OBJECTS): %.pb : %.proto
	protoc -o $@ $<

generate:
	$(JBPF_PROTOBUF_CLI_BIN) serde -s simple_input:simple_input; \
	rm -f *_serializer.c *.pb.c