// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include <unistd.h>
#include <assert.h>

#include "generated_data.h"
#include "simple_input.pb.h"

#include "jrtc_router_app_api.h"
#include "jrtc.h"

void*
jrtc_start_app(void* args)
{
    struct jrtc_app_env* env_ctx;
    jrtc_router_stream_id_t data_generator_codelet_sid;
    jrtc_router_stream_id_t app2_output_sid;
    jrtc_router_stream_id_t input_codelet_sid;
    jrtc_router_stream_id_t stream_id_app_input;
    int32_t agg_cnt = 0;
    int jbpf_agent_device_id = 1;
    int jrtc_device_id = 0;

    int num_rcv, res = 0;
    example_msg* data;
    simple_input_pb aggregate_counter = {};

    jrtc_router_data_entry_t data_entries[100] = {0};

    env_ctx = args;

    // Register stream_id request to get data from the data_generator codelet
    res = jrtc_router_generate_stream_id(
        &data_generator_codelet_sid,
        JRTC_ROUTER_REQ_DEST_ANY,
        JRTC_ROUTER_REQ_DEVICE_ID_ANY,
        "AdvancedExample1://jbpf_agent/data_generator_codeletset/codelet",
        "ringbuf");

    assert(res == 1);

    res = jrtc_router_channel_register_stream_id_req(env_ctx->dapp_ctx, data_generator_codelet_sid);

    assert(res == 1);

    // Register to get some data from the output channel of App2
    res = jrtc_router_generate_stream_id(
        &app2_output_sid, JRTC_ROUTER_REQ_DEST_ANY, jrtc_device_id, "AdvancedExample2://intraapp", "buffer");

    assert(res == 1);

    res = jrtc_router_channel_register_stream_id_req(env_ctx->dapp_ctx, app2_output_sid);

    assert(res == 1);

    // Create stream_id to send input data to the input codelet
    res = jrtc_router_generate_stream_id(
        &input_codelet_sid,
        JRTC_ROUTER_REQ_DEST_NONE,
        jbpf_agent_device_id,
        "AdvancedExample1://jbpf_agent/simple_input_pb_codeletset/codelet",
        "input_map");

    assert(res == 1);

    // Create input channel for receiving data from App2
    res = jrtc_router_generate_stream_id(
        &stream_id_app_input, JRTC_ROUTER_REQ_DEST_NONE, jrtc_device_id, "AdvancedExample1://intraapp", "buffer_input");

    assert(res == 1);

    dapp_channel_ctx_t ctx_input = jrtc_router_channel_create(
        env_ctx->dapp_ctx, false, 100, sizeof(simple_input_pb), stream_id_app_input, NULL, 0);

    assert(ctx_input);

    // Wait for the input channel to be created
    while (!jrtc_router_input_channel_exists(input_codelet_sid)) {
        sleep(1);
    }

    while (!atomic_load(&env_ctx->app_exit)) {

        num_rcv = jrtc_router_receive(env_ctx->dapp_ctx, data_entries, 100);
        if (num_rcv > 0) {
            for (int i = 0; i < num_rcv; i++) {

                if (jrtc_router_stream_id_matches_req(&data_entries[i].stream_id, &data_generator_codelet_sid)) {
                    // Data received from data generator codelet
                    data = data_entries[i].data;
                    agg_cnt += data->cnt;
                    printf("App1: Aggregate counter from codelet is %d\n", agg_cnt);
                    fflush(stdout);

                    // Send the aggregate counter back to the input codelet
                    res = jrtc_router_channel_send_input_msg(input_codelet_sid, &agg_cnt, sizeof(aggregate_counter));
                    assert(res == 0);
                } else if (jrtc_router_stream_id_matches_req(&data_entries[i].stream_id, &app2_output_sid)) {
                    // Data received from App2 output channel
                    simple_input_pb* appdata = data_entries[i].data;
                    printf(
                        "App1: Received aggregate counter %d from output channel of App2\n",
                        appdata->aggregate_counter);
                    fflush(stdout);
                } else if (jrtc_router_stream_id_matches_req(&data_entries[i].stream_id, &stream_id_app_input)) {
                    // Data received from App1 input channel (by App2)
                    simple_input_pb* appdata = data_entries[i].data;
                    printf(
                        "App1: Received aggregate counter %d from input channel of App1\n", appdata->aggregate_counter);
                    fflush(stdout);
                } else {
                    printf("App1: Got some unexpected message\n");
                    fflush(stdout);
                    assert(false);
                }
                jrtc_router_channel_release_buf(data);
            }
        }

        usleep(1000);
    }

    jrtc_router_channel_deregister_stream_id_req(env_ctx->dapp_ctx, data_generator_codelet_sid);

    return NULL;
}
