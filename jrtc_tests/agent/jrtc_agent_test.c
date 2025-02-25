// Copyright (c) Microsoft Corporation. All rights reserved.
/**
    This test tests the agent API by initializing the agent and stopping it.
    In particular, this test tests the following functions:
    - agent_init()
    - jrtc_router_init()
    - agent_stop()
*/
#include <assert.h>
#include <stdio.h>
#include "jrtc_agent.h"
#include "jbpf_io_channel.h"
#include "jbpf_io_channel_defs.h"
#include "jrtc_agent_defs.h"
#include "jrtc_logging.h"

#include "jrtc_router.h"
#include "jrtc_router_app_api.h"
#include "jrtc_router_stream_id.h"

#include "jrtc_logging.h"

#include "jbpf_io.h"
#include "jbpf_io_channel.h"
#include "jrtc_logging.h"

const char channel_name[] = "agent";
const char stream_path[] = "/tmp/jrtc";
struct jrtc_router_config config = {0};

int
start_router()
{
    config.thread_config.affinity_mask = 1 << 1;
    config.thread_config.has_affinity_mask = false;
    config.thread_config.has_sched_config = false;
    config.thread_config.sched_config.sched_policy = JRTC_ROUTER_DEADLINE;
    config.thread_config.sched_config.sched_priority = 99;
    config.thread_config.sched_config.sched_deadline = 30 * 1000 * 1000;
    config.thread_config.sched_config.sched_runtime = 10 * 1000 * 1000;

    config.thread_config.sched_config.sched_period = 30 * 1000 * 1000;

    strncpy(config.io_config.ipc_name, channel_name, 32);

    int res = jrtc_router_init(&config);

    if (res < 0) {
        jrtc_logger(JRTC_ERROR, "Failed to initialize router\n");
    } else {
        jrtc_logger(JRTC_INFO, "Router initialized successfully\n");
    }
    return res;
}

int
test_agent_api()
{
    size_t memory_size = 4194304;
    int device_id = 0;

    jrtc_logger(JRTC_INFO, "Initializing agent_init()...\n");
    if (agent_init((char*)channel_name, memory_size, device_id, (char*)stream_path) != 0) {
        fprintf(stderr, "Failed to initialize the agent.\n");
        return -1;
    }
    jrtc_logger(JRTC_INFO, "Agent initialized successfully.\n");

    // Stop the agent
    jrtc_logger(JRTC_INFO, "Stopping agent...\n");
    agent_stop();
    jrtc_logger(JRTC_INFO, "Agent stopped successfully.\n");
    return 0;
}

int
main()
{
    jrtc_logger(JRTC_INFO, "Starting the router...\n");
    assert(start_router() == 0);
    jrtc_logger(JRTC_INFO, "Starting test for agent API...\n");
    assert(test_agent_api() == 0);
    jrtc_logger(JRTC_INFO, "Test completed.\n");
    return 0;
}