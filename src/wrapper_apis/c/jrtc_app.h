// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef JRTC_APP_H
#define JRTC_APP_H

#include "jrtc.h"
#include "jrtc_router_app_api.h"

#ifdef __cplusplus
extern "C"
{
#endif

    // Forward declaration of the JrtcApp C++ class as an opaque pointer
    typedef struct JrtcApp JrtcApp;

    // Structure representing configuration for a Stream ID
    typedef struct
    {
        int destination;           // Destination identifier
        int device_id;             // Device identifier
        const char* stream_source; // Source of the stream
        const char* io_map;        // I/O mapping information
    } JrtcStreamIdCfg_t;

    // Structure representing the configuration of an application channel
    typedef struct
    {
        bool is_output; // Indicates if the channel is for output
        int num_elems;  // Number of elements in the channel
        int elem_size;  // Size of each element in bytes
    } JrtcAppChannelCfg_t;

    // Structure representing the configuration of a stream
    typedef struct
    {
        JrtcStreamIdCfg_t sid;           // Stream ID configuration
        bool is_rx;                      // Indicates if the stream is for receiving
        JrtcAppChannelCfg_t* appChannel; // Pointer to application channel configuration
    } JrtcStreamCfg_t;

    // Structure representing the overall application configuration
    typedef struct
    {
        char* context;                     // Application context (string)
        int q_size;                        // Queue size for processing
        int num_streams;                   // Number of streams
        JrtcStreamCfg_t* streams;          // Pointer to an array of stream configurations
        float initialization_timeout_secs; // Maximum time to wait for initialisation to complete.
        float sleep_timeout_secs;          // Sleep timeout in seconds
        float inactivity_timeout_secs;     // Inactivity timeout in seconds
    } JrtcAppCfg_t;

    // Callback function type for handling application events
    typedef void (*JrtcAppHandler)(bool success, int stream_idx, jrtc_router_data_entry_t* data, void* app_state);

    // Function to create a new JrtcApp instance
    // @param env_ctx - Environment context
    // @param app_cfg - Pointer to application configuration structure
    // @param app_handler - Event handler function pointer
    // @param app_state - Pointer to application state data
    // @return Pointer to the created JrtcApp instance
    JrtcApp*
    jrtc_app_create(struct jrtc_app_env* env_ctx, JrtcAppCfg_t* app_cfg, JrtcAppHandler app_handler, void* app_state);

    // Function to run the JrtcApp instance
    // @param app - Pointer to the JrtcApp instance
    void
    jrtc_app_run(JrtcApp* app);

    // Function to destroy a JrtcApp instance and free resources
    // @param app - Pointer to the JrtcApp instance to be destroyed
    void
    jrtc_app_destroy(JrtcApp* app);

    // abstraction wrapper for jrtc_router_channel_reserve_buf, using stream_index
    // @param app - Pointer to the JrtcApp instance to be destroyed
    // @param stream_idx - Index of the stream
    void*
    jrtc_app_router_channel_reserve_buf(JrtcApp* app, int stream_idx);

    // abstraction wrapper for jrtc_router_channel_send_output, using stream_index
    // @param app - Pointer to the JrtcApp instance to be destroyed
    // @param stream_idx - Index of the stream
    int
    jrtc_app_router_channel_send_output(JrtcApp* app, int stream_idx);

    // abstraction wrapper for jrtc_router_channel_send_output_msg, using stream_index
    // @param app - Pointer to the JrtcApp instance to be destroyed
    // @param stream_idx - Index of the stream
    int
    jrtc_app_router_channel_send_output_msg(JrtcApp* app, int stream_idx, void* data, size_t data_len);

    // abstraction wrapper for jrtc_app_router_channel_send_input_msg, using stream_index
    // @param app - Pointer to the JrtcApp instance to be destroyed
    // @param stream_idx - Index of the stream
    int
    jrtc_app_router_channel_send_input_msg(JrtcApp* app, uint stream_idx, void* data, size_t data_len);

#ifdef __cplusplus
}
#endif

#endif // JRTC_APP_H
