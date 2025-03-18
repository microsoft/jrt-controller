// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include <string.h>

#include "jbpf_defs.h"
#include "jbpf_helper.h"

#include "generated_data.h"

jbpf_ringbuf_map(ringbuf, example_msg, 10)

    struct jbpf_load_map_def SEC("maps") ringbuf_msg = {
        .type = JBPF_MAP_TYPE_ARRAY,
        .key_size = sizeof(int),
        .value_size = sizeof(example_msg),
        .max_entries = 1,
};

struct jbpf_load_map_def SEC("maps") counter = {
    .type = JBPF_MAP_TYPE_ARRAY,
    .key_size = sizeof(int),
    .value_size = sizeof(uint32_t),
    .max_entries = 1,
};

SEC("jbpf_ran_fapi")
uint64_t
jbpf_main(void* state)
{

    void* c;
    uint32_t cnt;
    uint64_t index = 0;

    /* inc counter in the map */
    c = jbpf_map_lookup_elem(&counter, &index);
    if (!c)
        return 0;
    cnt = *(uint32_t*)c;
    cnt = (cnt + 1) % 1024;
    *(uint32_t*)c = cnt;

    /* build a dummy example_msg response */
    example_msg* out;
    c = jbpf_map_lookup_reset_elem(&ringbuf_msg, &index);
    if (!c)
        return 0;
    out = (example_msg*)c;
    out->cnt = cnt;

    // send example_msg response
    jbpf_ringbuf_output(&ringbuf, out, sizeof(example_msg));

    return 0;
}
