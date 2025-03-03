// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include <assert.h>

#include "generated_data.pb.h" // Assuming relevant header for generated data
#include "simple_input.pb.h"   // Assuming relevant header for simple input

#include "jrtc_app.h" // Include the header for JrtcApp

// ##############################################
// Define the state variables for the application
typedef struct
{
    JrtcApp* app;

    // add custom fields below
    int32_t agg_cnt;
    int received_counter;
} AppStateVars_t;

// ############################
// enum for stream indexes
enum StreamIndex
{
    GENERATOR_PB_OUT_SIDX = 0,
    APP2_OUT_SIDX,
    APP1_IN_SIDX
};

// Function for handling received data (as in Python)
void
app_handler(bool timeout, int stream_idx, jrtc_router_data_entry_t* data_entry, void* s)
{

    AppStateVars_t* state = (AppStateVars_t*)s;

    if (timeout) {
        // Timeout processing (not implemented in this example)
    } else {

        switch (stream_idx) {

        case GENERATOR_PB_OUT_SIDX: {
            state->received_counter++;
            example_msg_pb* data = (example_msg_pb*)data_entry->data;
            state->agg_cnt += data->cnt;

            printf("App2: Aggregate counter from codelet is %d\n", state->agg_cnt);
            fflush(stdout);

            // Get a buffer to write the output data
            dapp_channel_ctx_t chan_ctx = jrtc_app_get_channel_context(state->app, APP2_OUT_SIDX);
            simple_input_pb* counter = (simple_input_pb*)jrtc_router_channel_reserve_buf(chan_ctx);
            assert(counter);
            counter->aggregate_counter = state->agg_cnt;
            // Send the data to the App2 output channel
            int res = jrtc_router_channel_send_output(chan_ctx);
            assert(res == 0 && "Error returned from jrtc_router_channel_send_output");

            // Send the data to the App1 input channel
            simple_input_pb aggregate_counter = {};
            aggregate_counter.aggregate_counter = state->agg_cnt;

            jrtc_router_stream_id_t stream = jrtc_app_get_stream(state->app, APP1_IN_SIDX);
            res = jrtc_router_channel_send_input_msg(stream, &aggregate_counter, sizeof(aggregate_counter));
            assert(res == 0 && "Failure in jrtc_router_channel_send_input_msg");
        } break;

        default:
            printf("App1: Got some unexpected message\n");
            fflush(stdout);
            assert(false);
        }

        if (state->received_counter % 5 == 0 && state->received_counter > 0) {
            printf("App2: Aggregate counter so far is %u\n", state->agg_cnt);
            // flush to standard output
            fflush(stdout);
        }
    }
}

// Main function to start the app (converted from jrtc_start_app)
void
jrtc_start_app(void* args)
{

    struct jrtc_app_env* env_ctx = (struct jrtc_app_env*)args;

    static const int jrtc_device_id = 0;

    // Configuration for the application
    const JrtcStreamCfg_t streams[] = {
        // GENERATOR_PB_OUT_SIDX = 0,
        {
            {JRTC_ROUTER_REQ_DEST_ANY,
             JRTC_ROUTER_REQ_DEVICE_ID_ANY,
             "AdvancedExample2://jbpf_agent/data_generator_pb_codeletset/codelet",
             "ringbuf"},
            true, // is_rx
            NULL  // No AppChannelCfg
        },
        // APP2_OUT_SIDX
        {{JRTC_ROUTER_REQ_DEST_NONE, jrtc_device_id, "AdvancedExample2://intraapp", "buffer"},
         false, // is_rx
         &(JrtcAppChannelCfg_t){true, 100, sizeof(simple_input_pb)}},
        // APP1_IN_SIDX
        {
            {JRTC_ROUTER_REQ_DEST_NONE, jrtc_device_id, "AdvancedExample1://intraapp", "buffer_input"},
            false, // is_rx
            NULL   // No AppChannelCfg
        }};

    const JrtcAppCfg_t app_cfg = {
        "AdvancedExample2",                   // context
        100,                                  // q_size
        sizeof(streams) / sizeof(streams[0]), // num_streams
        (JrtcStreamCfg_t*)streams,            // Pointer to the streams array
        0.1f,                                 // sleep_timeout_secs
        1.0f                                  // inactivity_timeout_secs
    };

    // Initialize the app
    AppStateVars_t state = {NULL, 0};
    state.app = jrtc_app_create(env_ctx, (JrtcAppCfg_t*)&app_cfg, app_handler, &state);

    // start app - This is blocking until the app exists
    jrtc_app_run(state.app);

    // clean up app resources
    jrtc_app_destroy(state.app);
}
