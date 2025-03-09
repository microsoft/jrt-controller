// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include <assert.h>

#include "generated_data.h"  // Assuming relevant header for generated data
#include "simple_input.pb.h" // Assuming relevant header for simple input

#include "jrtc_app.h" // Include the header for JrtcApp

// ##############################################
// Define the state variables for the application
typedef struct
{
    JrtcApp* app;

    // add custom fields below
    int32_t agg_cnt;
} AppStateVars_t;

// ############################
// enum for stream indexes
enum StreamIndex
{
    GENERATOR_OUT_SIDX = 0,
    APP2_OUT_SIDX,
    SIMPLE_INPUT_CODELET_IN_SIDX,
    APP1_IN_SIDX
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

        switch (stream_idx) {

        case GENERATOR_OUT_SIDX: {
            // Extract data from the received entry
            example_msg* data = (example_msg*)data_entry->data;
            state->agg_cnt += data->cnt;
            printf("App1: Aggregate counter from codelet is %d\n", state->agg_cnt);
            fflush(stdout);

            // Send the aggregate counter back to the input codelet
            simple_input_pb aggregate_counter = {};
            int res = jrtc_app_router_channel_send_input_msg(
                state->app, SIMPLE_INPUT_CODELET_IN_SIDX, &state->agg_cnt, sizeof(aggregate_counter));
            assert(res == 0 && "Failure in jrtc_router_channel_send_input_msg");
        } break;

        case APP2_OUT_SIDX: {
            // Data received from App2 output channel
            simple_input_pb* appdata = (simple_input_pb*)data_entry->data;
            printf("App1: Received aggregate counter %d from output channel of App2\n", appdata->aggregate_counter);
            fflush(stdout);
        } break;

        case APP1_IN_SIDX: {
            // Data received from App1 input channel (by App2)
            simple_input_pb* appdata = (simple_input_pb*)data_entry->data;
            printf("App1: Received aggregate counter %d from input channel of App1\n", appdata->aggregate_counter);
            fflush(stdout);

        } break;

        default:
            printf("App1: Got some unexpected message (stream_index=%d)\n", stream_idx);
            fflush(stdout);
            assert(false);
        }
    }
}

// Main function to start the app (converted from jrtc_start_app)
void
jrtc_start_app(void* args)
{

    struct jrtc_app_env* env_ctx = (struct jrtc_app_env*)args;

    static const int jbpf_agent_device_id = 1;
    static const int jrtc_device_id = 0;

    // Configuration for the application
    const JrtcStreamCfg_t streams[] = {
        // GENERATOR_OUT_SIDX
        {
            {JRTC_ROUTER_REQ_DEST_ANY,
             JRTC_ROUTER_REQ_DEVICE_ID_ANY,
             "AdvancedExample1://jbpf_agent/data_generator_codeletset/codelet",
             "ringbuf"},
            true, // is_rx
            NULL  // No AppChannelCfg
        },
        // APP2_OUT_SIDX
        {
            {JRTC_ROUTER_REQ_DEST_ANY, jrtc_device_id, "AdvancedExample2://intraapp", "buffer"},
            true, // is_rx
            NULL  // No AppChannelCfg
        },
        // SIMPLE_INPUT_CODELET_IN_SIDX
        {
            {JRTC_ROUTER_REQ_DEST_NONE,
             jbpf_agent_device_id,
             "AdvancedExample1://jbpf_agent/simple_input_pb_codeletset/codelet",
             "input_map"},
            false, // is_rx
            NULL   // No AppChannelCfg
        },
        // APP1_IN_SIDX
        {{JRTC_ROUTER_REQ_DEST_NONE, jrtc_device_id, "AdvancedExample1://intraapp", "buffer_input"},
         true, // is_rx
         &(JrtcAppChannelCfg_t){false, 100, sizeof(simple_input_pb)}}};

    const JrtcAppCfg_t app_cfg = {
        "AdvancedExample1",                   // context
        100,                                  // q_size
        sizeof(streams) / sizeof(streams[0]), // num_streams
        (JrtcStreamCfg_t*)streams,            // Pointer to the streams array
        10.0f,                                // initialization_timeout_secs
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
