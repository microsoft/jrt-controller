// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include <assert.h>

#include "generated_data.h" // relevant header for generated data
#include "simple_input.h"   // relevant header for simple input

#include "jrtc_app.h" // Include the header for JrtcCppApp

// ##########################################################################
// Define the state variables for the application
typedef struct
{
    JrtcApp* app;

    // add custom fields below
    int agg_cnt;
    int received_counter;
} AppStateVars_t;

// ############################
// enum for stream indexes
enum StreamIndex
{
    GENERATOR_OUT_SIDX = 0,
    SIMPLE_INPUT_IN_SIDX
};

// ##########################################################################
// Handler callback function (this function gets called by the C library)
void
app_handler(bool timeout, int stream_idx, jrtc_router_data_entry_t* data_entry, void* s)
{

    AppStateVars_t* state = (AppStateVars_t*)s;

    if (timeout) {
        // Timeout processing (not implemented in this example)
    } else {

        if (stream_idx == GENERATOR_OUT_SIDX) {
            state->received_counter += 1;

            // Extract data from the received entry
            example_msg* data = (example_msg*)data_entry->data;
            state->agg_cnt += data->cnt;
        }

        if (state->received_counter % 5 == 0 && state->received_counter > 0) {

            simple_input aggregate_counter = {0};

            aggregate_counter.aggregate_counter = state->agg_cnt;

            int res = jrtc_app_router_channel_send_input_msg(
                state->app, SIMPLE_INPUT_IN_SIDX, &aggregate_counter, sizeof(aggregate_counter));
            assert(res == 0 && "Failure returned from jrtc_router_channel_send_input_msg");

            printf("FirstExample: Aggregate counter so far is: %u \n", state->agg_cnt);
            fflush(stdout);
        }
    }
}

// ##########################################################################
// Main function to start the app (converted from jrtc_start_app)
void
jrtc_start_app(void* args)
{

    struct jrtc_app_env* env_ctx = (struct jrtc_app_env*)args;

    int jbpf_agent_device_id = 1;

    // Configuration for the application
    const JrtcStreamCfg_t streams[] = {// GENERATOR_OUT_SIDX
                                       {
                                           {JRTC_ROUTER_REQ_DEST_ANY,
                                            JRTC_ROUTER_REQ_DEVICE_ID_ANY,
                                            "FirstExample://jbpf_agent/data_generator_codeletset/codelet",
                                            "ringbuf"},
                                           true, // is_rx
                                           NULL  // No AppChannelCfg
                                       },
                                       // SIMPLE_INPUT_IN_SIDX
                                       {
                                           {JRTC_ROUTER_REQ_DEST_NONE,
                                            jbpf_agent_device_id,
                                            "FirstExample://jbpf_agent/simple_input_codeletset/codelet",
                                            "input_map"},
                                           false, // is_rx
                                           NULL   // No AppChannelCfg
                                       }};

    const JrtcAppCfg_t app_cfg = {
        "FirstExample",                       // context
        100,                                  // q_size
        sizeof(streams) / sizeof(streams[0]), // num_streams
        (JrtcStreamCfg_t*)streams,            // Pointer to the streams array
        10.0f,                                // initialization_timeout_secs
        0.1f,                                 // sleep_timeout_secs
        1.0f                                  // inactivity_timeout_secs
    };

    // Initialize the app
    AppStateVars_t state = {NULL, 0, 0};
    state.app = jrtc_app_create(env_ctx, (JrtcAppCfg_t*)&app_cfg, app_handler, &state);

    // start app - This is blocking until the app exists
    jrtc_app_run(state.app);

    // clean up app resources
    jrtc_app_destroy(state.app);
}
