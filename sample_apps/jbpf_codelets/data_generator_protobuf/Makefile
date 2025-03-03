# // Copyright (c) Microsoft Corporation. All rights reserved.
CC = clang
INC= -I${JRTC_OUT_DIR}/inc -I${NANO_PB}
CFLAGS = -O2 -target bpf -Wall -DJBPF_EXPERIMENTAL_FEATURES -DJBPF_DEBUG_ENABLED -D__x86_64__ -DPB_FIELD_32BIT=1 -DUSE_JBPF_PRINTF_HELPER -DJBPF_PRINTF_HELPER_ENABLED -DJBPF_DEBUG
JBPF_VERIFIER = ${JRTC_OUT_DIR}/bin/jbpf_verifier_cli
JBPF_PROTOBUF_CLI_BIN = ${JRTC_OUT_DIR}/bin/jbpf_protobuf_cli
SOURCES = $(wildcard *.c)
OBJECTS = $(SOURCES:.c=.o)
PROTO_SOURCES = $(wildcard *.proto)
PROTO_OBJECTS = $(PROTO_SOURCES:.proto=.pb)
GENERATED_HEADERS = $(PROTO_SOURCES:.proto=.pb.h)
PY_HEADERS = $(PROTO_SOURCES:.proto=_pb.py)

all: generate $(PROTO_OBJECTS) $(OBJECTS) $(GENERATED_HEADERS) $(PY_HEADERS)

$(OBJECTS): %.o : %.c
	$(CC) $(INC) $(CFLAGS) -c $<
	$(JBPF_VERIFIER) $@

clean:
	rm -f *.o *.pb.h *.so *.pb *.py

$(PROTO_OBJECTS): %.pb : %.proto
	protoc -o $@ $<

$(GENERATED_HEADERS): %.pb.h : %.proto
	protoc --proto_path=. --cpp_out=. $<

$(PY_HEADERS): %_pb.py : %.proto
	-include $(basename $<).options
	ctypesgen -I${NANO_PB} $(basename $<).pb.h -o $@   # Only runs if .oproto changes, but uses .pb.h as the input

generate:
	$(JBPF_PROTOBUF_CLI_BIN) serde -s generated_data:example_msg_pb; \
	rm *_serializer.c *.pb.c
