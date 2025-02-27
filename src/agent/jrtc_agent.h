// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#ifndef JRTC_AGENT_
#define JRTC_AGENT_

#include <stdio.h>

#include "jrtc_agent_defs.h"
#include "jbpf_io.h"
#include "jbpf_io_channel.h"

/**
 * @brief Initialize the agent
 * @ingroup agent
 * @param channel_name The name of the channel
 * @param memory_size The size of the memory
 * @param device_id The device id
 * @param stream_path The path to the stream
 * @return 0 on success, -1 on failure
 */
int
agent_init(char* channel_name, size_t memory_size, int device_id, char* stream_path);

/**
 * @brief Stop the agent
 * @ingroup agent
 */
void
agent_stop();

/**
 * @brief Create an input channel
 * @ingroup agent
 * @param fwd_dst The forward destination
 * @param num_elems The number of elements
 * @param schema_def The schema definition
 * @return The input channel
 */
jbpf_io_channel_t*
agent_create_input_channel(int fwd_dst, int num_elems, jrtc_agent_schema_definition* schema_def);

/**
 * @brief Create an output channel
 * @ingroup agent
 * @param fwd_dst The forward destination
 * @param num_elems The number of elements
 * @param schema_def The schema definition
 * @return The output channel
 */
jbpf_io_channel_t*
agent_create_output_channel(int fwd_dst, int num_elems, jrtc_agent_schema_definition* schema_def);

/**
 * @brief Destroy a channel
 * @ingroup agent
 * @param io_channel The channel to destroy
 */
void
agent_destroy_channel(jbpf_io_channel_t* io_channel);

#endif
