// Copyright (c) Microsoft Corporation. All rights reserved.
#include "jrtc_router_stream_id_int.h"
#include "jrtc_router_stream_id.h"

bool
__jrtc_router_stream_id_matches_req(const jrtc_router_stream_id_t* sid, const jrtc_router_stream_id_t* sid_req)
{
    return jrtc_router_stream_id_matches_req(sid, sid_req);
}
