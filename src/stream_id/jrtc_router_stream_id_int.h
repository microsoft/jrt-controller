// Copyright (c) Microsoft Corporation. All rights reserved.
// Wrapper functions which are exported
#ifndef JRTC_ROUTER_STREAM_ID_INT_
#define JRTC_ROUTER_STREAM_ID_INT_

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <arpa/inet.h>

#include "jrtc_router_bitmap.h"
#include "jbpf_io_channel_defs.h"
#include "jrtc_router_stream_id.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Generate a stream id from a stream id request
     * @ingroup stream_id
     * @param sid The stream id
     * @param sid_req The stream id request
     * @return true if the stream id matches the request, false otherwise
     */
    bool
    __jrtc_router_stream_id_matches_req(const jrtc_router_stream_id_t* sid, const jrtc_router_stream_id_t* sid_req);

#ifdef __cplusplus
}
#endif

#endif