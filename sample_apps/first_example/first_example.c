// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include <unistd.h>
#include <stdatomic.h>
#include <assert.h>

#include "generated_data.h"
#include "simple_input.h"

#include "jrtc_router_app_api.h"
#include "jrtc.h"

#define DATA_ENTRIES_SIZE (100)

void*
jrtc_start_app(void* args)
{
    struct jrtc_app_env* env_ctx;
    jrtc_router_stream_id_t codelet_sid;
    jrtc_router_stream_id_t control_input_sid;
    int32_t agg_cnt = 0;
    int jbpf_agent_device_id = 1;

    int num_rcv, res = 0;
    int received_counter;
    example_msg* data;
    simple_input aggregate_counter = {};

    jrtc_router_data_entry_t data_entries[100] = {0};

    env_ctx = args;

    // Register to get data from the data_generator_codelet
    res = jrtc_router_generate_stream_id(
        &codelet_sid,
        JRTC_ROUTER_REQ_DEST_ANY,
        JRTC_ROUTER_REQ_DEVICE_ID_ANY,
        "FirstExample://jbpf_agent/data_generator_codeletset/codelet1",
        "ringbuf");

    assert(res == 1);

    res = jrtc_router_channel_register_stream_id_req(env_ctx->dapp_ctx, codelet_sid);

    assert(res == 1);

    // Generate a stream id to send data to the control input codelet
    res = jrtc_router_generate_stream_id(
        &control_input_sid,
        JRTC_ROUTER_REQ_DEST_NONE,
        jbpf_agent_device_id,
        "FirstExample://jbpf_agent/unique_id_for_codelet_simple_input/codelet1",
        "input_map");

    received_counter = 0;

    // Wait for the input channel to be created
    while (!jrtc_router_input_channel_exists(control_input_sid)) {
        sleep(1);
    }

    while (!atomic_load(&env_ctx->app_exit)) {

        num_rcv = jrtc_router_receive(env_ctx->dapp_ctx, data_entries, DATA_ENTRIES_SIZE);
        if (num_rcv > 0) {
            for (int i = 0; i < num_rcv; i++) {

                // Check if the data matches the stream_id_req
                if (jrtc_router_stream_id_matches_req(&data_entries[i].stream_id, &codelet_sid)) {
                    received_counter++;
                    data = data_entries[i].data;
                    agg_cnt += data->cnt;
                }
                jrtc_router_channel_release_buf(data);
            }

            if (received_counter % 5 == 0 && received_counter > 0) {
                aggregate_counter.aggregate_counter = agg_cnt;
                jrtc_router_channel_send_input_msg(control_input_sid, &aggregate_counter, sizeof(aggregate_counter));
                printf("FirstExample: Aggregate counter so far is %u\n", agg_cnt);
                fflush(stdout);
            }
        }

        usleep(1000);
    }

    jrtc_router_channel_deregister_stream_id_req(env_ctx->dapp_ctx, codelet_sid);

    return NULL;
}
