# Copyright (c) Microsoft Corporation. All rights reserved.

CC = cc
INC = -I${JRTC_OUT_DIR}/inc
LDFLAGS := -L${JRTC_OUT_DIR}/lib -ljbpf -lck -lubpf -lmimalloc -lpthread -ldl -lrt

CFLAGS = -O3 -Wall -std=gnu11 -march=native -Werror -DJBPF_PERF_OPT
SOURCES = $(wildcard *.c)
OBJECTS = $(SOURCES:.c=.o)
TARGET = simple_agent_ipc

all: $(OBJECTS) simple_agent_ipc

simple_agent_ipc: simple_agent_ipc.o simple_agent_ipc.c
	$(CC) simple_agent_ipc.o -o simple_agent_ipc $(LDFLAGS)

$(OBJECTS): %.o : %.c
	$(CC) $(INC) $(CFLAGS) -c $<

clean :
	rm -f *.o simple_agent_ipc
