// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef JRTC_APP_HPP
#define JRTC_APP_HPP

#include <iostream>
#include <vector>
#include <string>
#include <optional>
#include <functional>
#include <any>
#include <memory>

#include "jrtc_app.h"

// JrtcApp class representing an application instance that manages streams
class JrtcApp
{
  public:
    // Structure representing a stream item with its ID, registration status, and channel context
    struct StreamItem
    {
        jrtc_router_stream_id_t sid; // Stream ID
        bool registered;             // Registration status of the stream
        std::unique_ptr<dapp_channel_ctx, decltype(&jrtc_router_channel_destroy)>
            chan_ctx; // Channel context associated with the stream

        StreamItem() : sid(), registered(false), chan_ctx(nullptr, jrtc_router_channel_destroy) {}
        StreamItem(jrtc_router_stream_id_t s, bool r, dapp_channel_ctx* c)
            : sid(s), registered(r), chan_ctx(c, jrtc_router_channel_destroy)
        {
        }
    };

    // Constructor initializing the JrtcApp instance
    // @param env_ctx - Environment context
    // @param app_cfg - Pointer to application configuration
    // @param app_handler - Function pointer for handling app events
    // @param app_state - Pointer to application state
    JrtcApp(struct jrtc_app_env* env_ctx, JrtcAppCfg_t* app_cfg, JrtcAppHandler app_handler, void* app_state);

    // Initializes the application instance
    int
    Init();

    // Cleans up and releases resources associated with the application
    void
    CleanUp();

    // Runs the application
    void
    run();

    // Retrieves the stream ID associated with a given index
    // @param stream_idx - Index of the stream
    // @return Stream ID
    jrtc_router_stream_id_t*
    get_stream(int stream_idx);

    // Retrieves the channel context for a given stream index
    // @param stream_idx - Index of the stream
    // @return Channel context
    dapp_channel_ctx_t
    get_chan_ctx(int stream_idx);

  private:
    struct jrtc_app_env* env_ctx;                             // Environment context
    JrtcAppCfg_t* app_cfg;                                    // Pointer to application configuration
    JrtcAppHandler app_handler;                               // Function pointer for handling application events
    void* app_state;                                          // Pointer to application state
    std::vector<StreamItem> stream_items;                     // Vector storing stream items
    std::chrono::steady_clock::time_point last_received_time; // Timestamp for last received event
};

#endif // JRTC_APP_HPP
