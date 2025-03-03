// Copyright (c) Microsoft Corporation. All rights reserved.

#include "jbpf_defs.h"
#include "jbpf_helper.h"
#include "simple_input.pb.h"

jbpf_control_input_map(input_map, simple_input_pb, 100)

    SEC("jbpf_ran_fapi") uint64_t jbpf_main(void* state)
{

    simple_input_pb in;

    while (jbpf_control_input_receive(&input_map, &in, sizeof(simple_input_pb)) > 0) {
        jbpf_printf_debug("Got aggregate value %u\n", in.aggregate_counter);
    }

    return 0;
}
