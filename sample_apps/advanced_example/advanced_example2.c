// Copyright (c) Microsoft Corporation. All rights reserved.
#include <unistd.h>
#include <assert.h>

#include "generated_data.pb.h"
#include "simple_input.pb.h"

#include "jrtc_router_app_api.h"
#include "jrtc.h"

void*
jrtc_start_app(void* args)
{
    struct jrtc_app_env* env_ctx;
    jrtc_router_stream_id_t codelet_stream_id;
    jrtc_router_stream_id_t stream_id_app_output;
    jrtc_router_stream_id_t stream_id_app_input;
    int32_t agg_cnt = 0;
    int jrtc_device_id = 0;

    int num_rcv, res = 0;
    int received_counter, sent_counter;
    example_msg* data;
    simple_input aggregate_counter = {};

    jrtc_router_data_entry_t data_entries[100] = {0};

    env_ctx = args;

    // Register stream_id request to get data from the data_generator_pb codelet
    res = jrtc_router_generate_stream_id(
        &codelet_stream_id,
        JRTC_ROUTER_REQ_DEST_ANY,
        JRTC_ROUTER_REQ_DEVICE_ID_ANY,
        "AdvancedExample2://jbpf_agent/data_generator_pb_codeletset/codelet",
        "ringbuf");

    assert(res == 1);

    res = jrtc_router_channel_register_stream_id_req(env_ctx->dapp_ctx, codelet_stream_id);

    assert(res == 1);

    // Create an output channel, to send data to App1
    res = jrtc_router_generate_stream_id(
        &stream_id_app_output, JRTC_ROUTER_REQ_DEST_NONE, jrtc_device_id, "AdvancedExample2://intraapp", "buffer");

    assert(res == 1);

    dapp_channel_ctx_t output_channel_ctx = jrtc_router_channel_create(
        env_ctx->dapp_ctx, true, 100, sizeof(aggregate_counter), stream_id_app_output, NULL, 0);

    assert(output_channel_ctx);

    // Create a stream_id to send data to the input channel of App1
    res = jrtc_router_generate_stream_id(
        &stream_id_app_input, JRTC_ROUTER_REQ_DEST_NONE, jrtc_device_id, "AdvancedExample1://intraapp", "buffer_input");

    assert(res);

    received_counter = 0;
    sent_counter = 0;

    // Wait for the input channel to be created
    while (!jrtc_router_input_channel_exists(stream_id_app_input)) {
        sleep(1);
    }

    while (!atomic_load(&env_ctx->app_exit)) {

        num_rcv = jrtc_router_receive(env_ctx->dapp_ctx, data_entries, 100);
        if (num_rcv > 0) {
            for (int i = 0; i < num_rcv; i++) {

                // Check if the data matches the codelet_stream_id
                if (jrtc_router_stream_id_matches_req(&data_entries[i].stream_id, &codelet_stream_id)) {
                    received_counter++;
                    data = data_entries[i].data;
                    agg_cnt += data->cnt;

                    printf("App2: Aggregate counter from codelet is %d\n", agg_cnt);

                    // Get a buffer to write the output data
                    simple_input* counter = jrtc_router_channel_reserve_buf(output_channel_ctx);
                    assert(counter);
                    counter->aggregate_counter = agg_cnt;
                    // Send the data to the output channel
                    res = jrtc_router_channel_send_output(output_channel_ctx);
                    assert(res == 0);

                    aggregate_counter.aggregate_counter = agg_cnt;
                    jrtc_router_channel_send_input_msg(
                        stream_id_app_input, &aggregate_counter, sizeof(aggregate_counter));

                    sent_counter++;
                } else {
                    printf("App2: Got some unexpected message\n");
                }
                jrtc_router_channel_release_buf(data);
            }

            if (received_counter % 5 == 0 && received_counter > 0) {
                printf("App2: Aggregate counter so far is %u\n", agg_cnt);
                // flush to standard output
                fflush(stdout);
            }
        }

        usleep(1000);
    }

    jrtc_router_channel_deregister_stream_id_req(env_ctx->dapp_ctx, codelet_stream_id);

    return NULL;
}
