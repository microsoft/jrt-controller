// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include <stdio.h>

#include "jrtc_agent.h"
#include "jrtc_agent_defs.h"
#include "jrtc_router_stream_id.h"

int global_device_id;
char* global_stream_path;

int
agent_init(char* channel_name, size_t memory_size, int device_id, char* stream_path)
{
    if (strlen(channel_name) + 1 > JBPF_IO_IPC_MAX_NAMELEN)
        return -1;

    struct jbpf_io_config io_config = {0};

    io_config.type = JBPF_IO_IPC_SECONDARY;
    io_config.ipc_config.mem_cfg.memory_size = memory_size;

    strncpy(
        io_config.ipc_config.addr.jbpf_io_ipc_name,
        channel_name,
        sizeof(io_config.ipc_config.addr.jbpf_io_ipc_name) - 1);
    io_config.ipc_config.addr.jbpf_io_ipc_name[sizeof(io_config.ipc_config.addr.jbpf_io_ipc_name) - 1] =
        '\0'; // Ensure null termination

    strncpy(io_config.jbpf_path, JBPF_DEFAULT_RUN_PATH, JBPF_RUN_PATH_LEN - 1);
    io_config.jbpf_path[JBPF_RUN_PATH_LEN - 1] = '\0';
    strncpy(io_config.jbpf_namespace, JBPF_DEFAULT_NAMESPACE, JBPF_NAMESPACE_LEN - 1);
    io_config.jbpf_namespace[JBPF_NAMESPACE_LEN - 1] = '\0';

    struct jbpf_io_ctx* io_ctx = jbpf_io_init(&io_config);

    if (io_ctx == NULL)
        return -2;

    global_device_id = device_id;

    global_stream_path = strdup(stream_path);
    if (global_stream_path == NULL)
        return -3;

    jbpf_io_register_thread(

    );

    return 0;
}

void
agent_stop()
{
    jbpf_io_stop();
    free(global_stream_path);
}

struct jbpf_io_stream_id
generate_stream_id(int fwd_dst, int device_id, char* stream_path, jrtc_agent_schema_definition* schema_def)
{
    struct jbpf_io_stream_id out = {0};
    jrtc_router_stream_id_t sid;
    jrtc_router_generate_stream_id(&sid, fwd_dst, device_id, stream_path, schema_def->name);
    memcpy(out.id, sid.id, JBPF_IO_STREAM_ID_LEN);
    return out;
}

jbpf_io_channel_t*
agent_create_input_channel(int fwd_dst, int num_elems, jrtc_agent_schema_definition* schema_def)
{
    jbpf_io_ctx_t* io_ctx = jbpf_io_get_ctx();
    if (io_ctx == NULL)
        return NULL;
    return jbpf_io_create_channel(
        io_ctx,
        JBPF_IO_CHANNEL_INPUT,
        JBPF_IO_CHANNEL_QUEUE,
        num_elems,
        schema_def->elem_size,
        generate_stream_id(fwd_dst, global_device_id, global_stream_path, schema_def),
        schema_def->decoder,
        schema_def->decoder_size);
}

jbpf_io_channel_t*
agent_create_output_channel(int fwd_dst, int num_elems, jrtc_agent_schema_definition* schema_def)
{
    jbpf_io_ctx_t* io_ctx = jbpf_io_get_ctx();
    if (io_ctx == NULL)
        return NULL;
    return jbpf_io_create_channel(
        io_ctx,
        JBPF_IO_CHANNEL_OUTPUT,
        JBPF_IO_CHANNEL_QUEUE,
        num_elems,
        schema_def->elem_size,
        generate_stream_id(fwd_dst, global_device_id, global_stream_path, schema_def),
        schema_def->encoder,
        schema_def->encoder_size);
}

void
agent_destroy_channel(jbpf_io_channel_t* io_channel)
{
    jbpf_io_ctx_t* io_ctx = jbpf_io_get_ctx();
    if (io_ctx != NULL)
        jbpf_io_destroy_channel(io_ctx, io_channel);
}
