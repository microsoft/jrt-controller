// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#ifndef JRTC_AGENT_DEFS_
#define JRTC_AGENT_DEFS_

#include <stdio.h>

#include "jbpf_io_channel.h"

/**
 * @defgroup agent
 * @brief The agent module
 *
 * @defgroup controller
 * @brief The controller module
 *
 * @defgroup logger
 * @brief The logger module
 *
 * @defgroup rest_api_lib
 * @brief The rest api library
 *
 * @defgroup router
 * @brief The router module
 *
 * @defgroup stream_id
 * @brief The stream id module
 */

/**
 * @brief The jrtc_agent_schema_definition struct
 * @ingroup agent
 * This struct is used to define the schema of the data that is being sent to the jrtc agent.
 * The schema is defined by the decoder and encoder functions that are used to encode and decode the data.
 */
typedef struct jrtc_agent_schema_definition
{
    char* decoder;
    char* encoder;
    char* name;
    size_t decoder_size;
    size_t elem_size;
    size_t encoder_size;
} jrtc_agent_schema_definition;

#endif
