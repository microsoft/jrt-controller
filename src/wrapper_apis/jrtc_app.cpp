// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include <chrono>
#include <thread>
#include <cassert>
#include <iostream>
#include "jrtc_app.hpp"

// ###########################################################
// Overloaded ostream operators for logging config structures
std::ostream&
operator<<(std::ostream& os, const JrtcStreamIdCfg_t& sid)
{
    os << "JrtcStreamIdCfg_t(destination=" << sid.destination << ", device_id=" << sid.device_id << ", stream_source='"
       << (sid.stream_source ? sid.stream_source : "nullptr") << "', io_map='" << (sid.io_map ? sid.io_map : "nullptr")
       << "')";
    return os;
}

std::ostream&
operator<<(std::ostream& os, const JrtcAppChannelCfg_t& chan)
{
    os << "JrtcAppChannelCfg_t(is_output=" << (chan.is_output ? "true" : "false") << ", num_elems=" << chan.num_elems
       << ", elem_size=" << chan.elem_size << ")";
    return os;
}

std::ostream&
operator<<(std::ostream& os, const JrtcStreamCfg_t& stream)
{
    os << "JrtcStreamCfg_t(sid=" << stream.sid << ", is_rx=" << (stream.is_rx ? "true" : "false") << ", appChannel=";
    if (stream.appChannel) {
        os << *stream.appChannel;
    } else {
        os << "nullptr";
    }
    os << ")";
    return os;
}

std::ostream&
operator<<(std::ostream& os, JrtcAppCfg_t* app_cfg)
{
    os << "JrtcAppCfg_t(context='" << (app_cfg->context ? app_cfg->context : "nullptr")
       << "', q_size=" << app_cfg->q_size << ", num_streams=" << app_cfg->num_streams
       << ", sleep_timeout_secs=" << app_cfg->sleep_timeout_secs
       << ", inactivity_timeout_secs=" << app_cfg->inactivity_timeout_secs << ", streams=[";

    for (int i = 0; i < app_cfg->num_streams; i++) {
        os << app_cfg->streams[i];
        if (i < app_cfg->num_streams - 1) {
            os << ", ";
        }
    }
    os << "])";
    return os;
}

// ###########################################################
// Constructor: Initializes JrtcApp instance
JrtcApp::JrtcApp(struct jrtc_app_env* env_ctx, JrtcAppCfg_t* app_cfg, JrtcAppHandler app_handler, void* app_state)
    : env_ctx(env_ctx), app_cfg(app_cfg), app_handler(app_handler), app_state(app_state), stream_items(),
      last_received_time(std::chrono::steady_clock::now())
{
    Init();
}

// ###########################################################
// Initializes streams and channels based on configuration
void
JrtcApp::Init()
{
    last_received_time = std::chrono::steady_clock::now();
    std::cout << app_cfg->context << "::  app_cfg: " << app_cfg << std::endl;

    for (int i = 0; i < app_cfg->num_streams; ++i) {
        auto& s = app_cfg->streams[i];
        StreamItem si{jrtc_router_stream_id_t(), false, nullptr};
        int res;
        jrtc_router_stream_id_t sid;

        // Generate stream ID
        res =
            jrtc_router_generate_stream_id(&sid, s.sid.destination, s.sid.device_id, s.sid.stream_source, s.sid.io_map);
        assert(res == 1);
        si.sid = sid;

        // Create channel if needed
        if (s.appChannel) {
            si.chan_ctx = jrtc_router_channel_create(
                env_ctx->dapp_ctx,
                s.appChannel->is_output,
                s.appChannel->num_elems,
                s.appChannel->elem_size,
                si.sid,
                0,
                0);
            assert(si.chan_ctx);
        }

        // Register stream if it is for reception
        if (s.is_rx) {
            res = jrtc_router_channel_register_stream_id_req(env_ctx->dapp_ctx, si.sid);
            assert(res == 1);
            si.registered = true;
        }

        stream_items.push_back(si);
    }

    // Now that all this app's channels have been created, check channels which the app might transmit to are created
    for (int i = 0; i < app_cfg->num_streams; ++i) {
        auto& s = app_cfg->streams[i];
        auto& si = stream_items[i];

        if ((!s.is_rx) && (!si.chan_ctx)) {
            int k = 0;
            while (!jrtc_router_input_channel_exists(si.sid)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleeps for 100ms
                if (k++ == 10) {
                    std::cout << app_cfg->context << "::  Waiting for creation of : " << s.sid << std::endl;
                    k = 0;
                }
            }
        }
    }
}

// ###########################################################
// Cleans up resources and deregisters streams
void
JrtcApp::CleanUp()
{
    for (auto& si : stream_items) {
        if (si.registered) {
            jrtc_router_channel_deregister_stream_id_req(env_ctx->dapp_ctx, si.sid);
        }
        if (si.chan_ctx) {
            jrtc_router_channel_destroy(si.chan_ctx);
        }
    }
}

// ###########################################################
// Runs the main application loop
void
JrtcApp::run()
{
    std::vector<jrtc_router_data_entry_t> data_entries(app_cfg->q_size, {0});
    while (!atomic_load(&env_ctx->app_exit)) {
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration<double>(now - last_received_time).count() > app_cfg->inactivity_timeout_secs) {
            app_handler(true, -1, nullptr, app_state);
            last_received_time = now;
        }

        auto num_rcv = jrtc_router_receive(env_ctx->dapp_ctx, data_entries.data(), app_cfg->q_size);
        for (int i = 0; i < num_rcv; ++i) {

            // find index which matches stream, if any
            bool found = false;
            int sidx = 0;
            for (; sidx < app_cfg->num_streams; sidx++) {
                auto& s = app_cfg->streams[sidx];
                auto& si = stream_items[sidx];
                if ((s.is_rx) && (jrtc_router_stream_id_matches_req(&data_entries[i].stream_id, &si.sid))) {
                    found = true;
                    break;
                }
            }
            // if stream found, call handler
            if (found) {
                app_handler(false, sidx, &data_entries[i], app_state);
            }
            jrtc_router_channel_release_buf(data_entries[i].data);
            last_received_time = std::chrono::steady_clock::now();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(app_cfg->sleep_timeout_secs * 1000)));
    }
    CleanUp();
}

// ###########################################################
// Retrieves the stream ID associated with a given index
jrtc_router_stream_id_t
JrtcApp::get_stream(int stream_idx)
{
    return (static_cast<size_t>(stream_idx) < stream_items.size()) ? stream_items[stream_idx].sid
                                                                   : jrtc_router_stream_id_t();
}

// ###########################################################
// Retrieves the channel context for a given index
dapp_channel_ctx_t
JrtcApp::get_chan_ctx(int stream_idx)
{
    return (static_cast<size_t>(stream_idx) < stream_items.size()) ? stream_items[stream_idx].chan_ctx : nullptr;
}

// ###########################################################
// C API wrapper functions
extern "C"
{
    JrtcApp*
    jrtc_app_create(struct jrtc_app_env* env_ctx, JrtcAppCfg_t* app_cfg, JrtcAppHandler app_handler, void* app_state)
    {
        return new JrtcApp(env_ctx, app_cfg, app_handler, app_state);
    }
    void
    jrtc_app_run(JrtcApp* app)
    {
        if (app)
            app->run();
    }
    void
    jrtc_app_destroy(JrtcApp* app)
    {
        delete app;
    }
    jrtc_router_stream_id_t
    jrtc_app_get_stream(JrtcApp* app, int stream_idx)
    {
        return app->get_stream(stream_idx);
    }
    dapp_channel_ctx_t
    jrtc_app_get_channel_context(JrtcApp* app, int stream_idx)
    {
        return app->get_chan_ctx(stream_idx);
    }
}
