// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#define _GNU_SOURCE
#include <linux/sched.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jbpf_io.h"
#include "jbpf_io_channel.h"
#include "jbpf_io_hash.h"
#include "jbpf_io_utils.h"

// Forward declaration
struct jrtc_config;

#include "jrtc_router.h"
#include "jrtc_router_app_api.h"
#include "jrtc_router_int.h"

#include "jrtc_logging.h"
#include "jbpf_mempool.h"
#include "jrtc_config_int.h"

struct jrtc_router_ctx g_router_ctx;

struct dapp_channel_ctx
{
    struct jbpf_io_channel* io_channel;
    jrtc_router_stream_id_t stream_id;
    dapp_router_ctx_t app_ctx;
    bool is_output;
};

CK_EPOCH_CONTAINER(jrtc_router_req_entry_t, epoch_entry, epoch_container)

#define round_up_pow_of_two(x) \
    ({                         \
        uint32_t v = x;        \
        v--;                   \
        v |= v >> 1;           \
        v |= v >> 2;           \
        v |= v >> 4;           \
        v |= v >> 8;           \
        v |= v >> 16;          \
        ++v;                   \
    })

// This is a mask we use to check for wildcards.
// It makes some of the fields of the stream_id to use their ANY value
static struct jrtc_router_stream_id_int lookup_mask[JRTC_ROUTER_NUM_REQ_LOOKUPS] = {
    {.ver = 0, .fwd_dst = 0, .device_id = 0, .stream_path = 0, .stream_name = 0},
    {.ver = 0, .fwd_dst = 0, .device_id = 0, .stream_path = 0, .stream_name = JRTC_ROUTER_STREAM_NAME_ANY},
    {.ver = 0, .fwd_dst = 0, .device_id = 0, .stream_path = JRTC_ROUTER_STREAM_PATH_ANY, .stream_name = 0},
    {.ver = 0, .fwd_dst = 0, .device_id = JRTC_ROUTER_DEVICE_ID_ANY, .stream_path = 0, .stream_name = 0},
    {.ver = 0, .fwd_dst = JRTC_ROUTER_DEST_ANY, .device_id = 0, .stream_path = 0, .stream_name = 0},
    {.ver = 0,
     .fwd_dst = 0,
     .device_id = 0,
     .stream_path = JRTC_ROUTER_STREAM_PATH_ANY,
     .stream_name = JRTC_ROUTER_STREAM_NAME_ANY},
    {.ver = 0,
     .fwd_dst = 0,
     .device_id = JRTC_ROUTER_DEVICE_ID_ANY,
     .stream_path = 0,
     .stream_name = JRTC_ROUTER_STREAM_NAME_ANY},
    {.ver = 0,
     .fwd_dst = JRTC_ROUTER_DEST_ANY,
     .device_id = 0,
     .stream_path = 0,
     .stream_name = JRTC_ROUTER_STREAM_NAME_ANY},
    {.ver = 0,
     .fwd_dst = 0,
     .device_id = JRTC_ROUTER_DEVICE_ID_ANY,
     .stream_path = JRTC_ROUTER_STREAM_PATH_ANY,
     .stream_name = 0},
    {.ver = 0,
     .fwd_dst = JRTC_ROUTER_DEST_ANY,
     .device_id = 0,
     .stream_path = JRTC_ROUTER_STREAM_PATH_ANY,
     .stream_name = 0},
    {.ver = 0,
     .fwd_dst = JRTC_ROUTER_DEST_ANY,
     .device_id = JRTC_ROUTER_DEVICE_ID_ANY,
     .stream_path = 0,
     .stream_name = 0},
    {.ver = 0,
     .fwd_dst = 0,
     .device_id = JRTC_ROUTER_DEVICE_ID_ANY,
     .stream_path = JRTC_ROUTER_STREAM_PATH_ANY,
     .stream_name = JRTC_ROUTER_STREAM_NAME_ANY},
    {.ver = 0,
     .fwd_dst = JRTC_ROUTER_DEST_ANY,
     .device_id = 0,
     .stream_path = JRTC_ROUTER_STREAM_PATH_ANY,
     .stream_name = JRTC_ROUTER_STREAM_NAME_ANY},
    {.ver = 0,
     .fwd_dst = JRTC_ROUTER_DEST_ANY,
     .device_id = JRTC_ROUTER_DEVICE_ID_ANY,
     .stream_path = 0,
     .stream_name = JRTC_ROUTER_STREAM_NAME_ANY},
    {.ver = 0,
     .fwd_dst = JRTC_ROUTER_DEST_ANY,
     .device_id = JRTC_ROUTER_DEVICE_ID_ANY,
     .stream_path = JRTC_ROUTER_STREAM_PATH_ANY,
     .stream_name = 0},
    {.ver = 0,
     .fwd_dst = JRTC_ROUTER_DEST_ANY,
     .device_id = JRTC_ROUTER_DEVICE_ID_ANY,
     .stream_path = JRTC_ROUTER_STREAM_PATH_ANY,
     .stream_name = JRTC_ROUTER_STREAM_NAME_ANY},
};

struct router_thread_args
{
    struct jrtc_router_ctx* router_ctx;
    struct jrtc_router_config* config;
};

static int
sched_setattr(pid_t pid, const struct sched_attr* attr, unsigned int flags)
{
    return syscall(__NR_sched_setattr, pid, attr, flags);
}

static void
ht_hash_wrapper(struct ck_ht_hash* h, const void* key, size_t length, uint64_t seed)
{

    h->value = (unsigned long)MurmurHash64A(key, length, seed);
}

static void*
ht_malloc(size_t r)
{
    return jbpf_malloc(r);
}

static void
ht_free(void* p, size_t b, bool r)
{
    (void)b;
    (void)r;
    jbpf_free(p);
}

static struct ck_malloc ht_allocator = {.malloc = ht_malloc, .free = ht_free};

static void
_stream_id_req_destructor(ck_epoch_entry_t* p)
{
    jrtc_router_req_entry_t* req_entry = epoch_container(p);

    jbpf_free(req_entry->bitmap);
    jbpf_free(req_entry);
}

void
_jrtc_router_forward_msgs(
    struct jbpf_io_channel* io_channel, struct jbpf_io_stream_id* stream_id, void** bufs, int num_bufs, void* ctx)
{
    jbpf_mbuf_t* mbuf;
    jrtc_router_data_entry_t* data_entry;
    jrtc_router_req_entry_t* req_entry;
    jrtc_router_ctx_t router_ctx;
    jrtc_router_stream_id_t* sid;
    jrtc_router_stream_id_t lookup_id;
    ck_ht_hash_t h_req;
    ck_ht_entry_t req_table_entry;
    ck_bitmap_t* lookup_res;
    ck_bitmap_iterator_t iter;
    struct dapp_router_ctx* dapp;
    void* ptr;
    unsigned int app_id;

    if (!io_channel || num_bufs <= 0) {
        return;
    }

    // print_stream_id("We have messages to process from stream_id %s\n",
    // stream_id);

    router_ctx = ctx;
    sid = (jrtc_router_stream_id_t*)stream_id;
    lookup_res = router_ctx->req_table.lookup_result;

    ck_bitmap_clear(lookup_res);

    ck_epoch_begin(&router_ctx->req_table.router_epoch_record, NULL);

    for (int i = 0; i < JRTC_ROUTER_NUM_REQ_LOOKUPS; i++) {
        lookup_id = *sid;
        _jrtc_router_stream_id_apply_fwd_dst_mask(lookup_id.id, lookup_mask[i].fwd_dst);
        _jrtc_router_stream_id_apply_device_id_mask(lookup_id.id, lookup_mask[i].device_id);
        _jrtc_router_stream_id_apply_stream_path_mask(lookup_id.id, lookup_mask[i].stream_path);
        _jrtc_router_stream_id_apply_stream_name_mask(lookup_id.id, lookup_mask[i].stream_name);

        ck_ht_hash(&h_req, &router_ctx->req_table.reqs, &lookup_id, JRTC_ROUTER_STREAM_ID_BYTE_LEN);
        ck_ht_entry_key_set(&req_table_entry, &lookup_id, JRTC_ROUTER_STREAM_ID_BYTE_LEN);

        // print_stream_id("Now checking %s\n", &lookup_id);

        // Check if any apps have made this request and if so, add to the list of
        // apps
        if (ck_ht_get_spmc(&router_ctx->req_table.reqs, h_req, &req_table_entry)) {
            req_entry = ck_ht_entry_value(&req_table_entry);
            ck_bitmap_union(lookup_res, req_entry->bitmap);
        }
    }

    ck_epoch_end(&router_ctx->req_table.router_epoch_record, NULL);

    ck_bitmap_iterator_init(&iter, lookup_res);

    for (int i = 0; i < num_bufs; i++) {
        for (int j = 0; ck_bitmap_next(lookup_res, &iter, &app_id) == true; j++) {
            // Place to the ringbuffers of all the apps and release
            dapp = router_ctx->app_metadata.ctx[app_id];

            if (!dapp) {
                continue;
            }

            mbuf = jbpf_mbuf_alloc(dapp->data_entry_pool);

            if (!mbuf) {
                continue;
            }

            data_entry = (jrtc_router_data_entry_t*)mbuf->data;

            ptr = jbpf_io_channel_share_data_ptr(bufs[i]);

            data_entry->data = ptr;
            data_entry->stream_id = *sid;

            ck_ring_enqueue_spsc(&dapp->ring, dapp->ringbuffer, data_entry);
        }
        jbpf_io_channel_release_buf(bufs[i]);
    }
}

static void
_jrtc_router_handle_incoming_msgs()
{
    jrtc_router_ctx_t router_ctx;
    struct jbpf_io_ctx* io_ctx;

    router_ctx = jrtc_router_get_ctx();

    io_ctx = router_ctx->io_ctx;

    if (!io_ctx || io_ctx->io_type == JBPF_IO_IPC_SECONDARY)
        return;

    jbpf_io_channel_handle_out_bufs(io_ctx, _jrtc_router_forward_msgs, router_ctx);
}

void*
jrtc_router_thread_start(void* args)
{

    struct router_thread_args* th_args;
    struct jrtc_router_config* config;
    struct jrtc_router_ctx* ctx;

    th_args = args;
    config = th_args->config;
    ctx = th_args->router_ctx;
    free(th_args);

    if (config->thread_config.has_sched_config && config->thread_config.has_affinity_mask) {
        if (config->thread_config.sched_config.sched_policy == JRTC_ROUTER_DEADLINE) {
            jrtc_logger(
                JRTC_WARN,
                "WARNING: SCHED_DEADLINE policy cannot be used in "
                "conjunction with affinitized threads\n");
        }
    }

    if (config->thread_config.has_affinity_mask) {
        jrtc_router_set_cpu_affinity(ctx, config->thread_config.affinity_mask);
    }

    if (config->thread_config.has_sched_config) {
        jrtc_router_set_scheduler(ctx, &config->thread_config.sched_config);
    }

    if (pthread_setname_np(pthread_self(), "jrtc_router")) {
        jrtc_logger(JRTC_ERROR, "Error in setting app name to %s\n", "jrtc_router");
    } else {
        jrtc_logger(JRTC_INFO, "Set name %s successfully\n", "jrtc_router");
    }

    jbpf_io_register_thread();

    while (1) {
        _jrtc_router_handle_incoming_msgs();
        usleep(5);
    }

    return NULL;
}

int
jrtc_router_init(struct jrtc_config* config)
{

    struct router_thread_args* thread_args;
    unsigned int bytes;

    unsigned int mode = CK_HT_MODE_BYTESTRING;

    if (!config) {
        return -1;
    }

    jrtc_logger(
        JRTC_INFO,
        "Initializing router with namespace %s and path %s, ipc_name %s\n",
        config->jbpf_io_config.jbpf_namespace,
        config->jbpf_io_config.jbpf_path,
        config->jrtc_router_config.io_config.ipc_name);

    g_router_ctx.io_ctx = jbpf_io_init(&config->jbpf_io_config);
    if (g_router_ctx.io_ctx == NULL) {
        jrtc_logger(JRTC_ERROR, "Could not initialize IO successfully\n");
        goto error_io_init;
    } else {
        jrtc_logger(JRTC_INFO, "IO initilized successfully\n");
    }

    thread_args = malloc(sizeof(struct router_thread_args));
    if (!thread_args) {
        jrtc_logger(JRTC_ERROR, "Error allocating memory for thread args\n");
        goto error_router_thread;
    }

    thread_args->config = &config->jrtc_router_config;
    thread_args->router_ctx = &g_router_ctx;

    // Initialize the request tables and the app metadata
    ck_spinlock_init(&g_router_ctx.req_table.lock);

    ck_epoch_init(&g_router_ctx.req_table.router_epoch);
    ck_epoch_register(&g_router_ctx.req_table.router_epoch, &g_router_ctx.req_table.router_epoch_record, NULL);
    for (int i = 0; i < JRTC_ROUTER_MAX_NUM_APPS; i++) {
        ck_epoch_register(&g_router_ctx.req_table.router_epoch, &g_router_ctx.req_table.app_epoch_record[i], NULL);
    }

    bytes = ck_bitmap_size(JRTC_ROUTER_MAX_NUM_APPS);

    g_router_ctx.req_table.lookup_result = jbpf_malloc(bytes);

    if (!g_router_ctx.req_table.lookup_result) {
        goto error_thread_init;
    }

    ck_bitmap_init(g_router_ctx.req_table.lookup_result, JRTC_ROUTER_MAX_NUM_APPS, false);

    if (!ck_ht_init(
            &g_router_ctx.req_table.reqs,
            mode,
            ht_hash_wrapper,
            &ht_allocator,
            JRTC_ROUTER_INIT_NUM_REQ_ENTRIES,
            6602834)) {

        goto error_lookup_res;
    }

    g_router_ctx.app_metadata.app_bitmap = jbpf_malloc(bytes);

    if (!g_router_ctx.app_metadata.app_bitmap) {
        goto error_app_metadata_init;
    }

    ck_bitmap_init(g_router_ctx.app_metadata.app_bitmap, JRTC_ROUTER_MAX_NUM_APPS, false);

    if (pthread_create(
            &g_router_ctx.th_ctx.jrtc_router_thread_id, NULL, jrtc_router_thread_start, (void*)thread_args) != 0) {
        jrtc_logger(JRTC_ERROR, "Error creating router thread\n");
        goto error_thread_init;
    }

    return 0;

error_app_metadata_init:
    ck_ht_destroy(&g_router_ctx.req_table.reqs);
error_lookup_res:
    jbpf_free(g_router_ctx.req_table.lookup_result);
error_thread_init:
    free(thread_args);
error_router_thread:
    jbpf_io_stop();
error_io_init:
    return -1;
}

int
jrtc_router_stop()
{
    // TODO
    // pthread_join(g_router_ctx.th_ctx.jrtc_router_thread_id, NULL);
    return 0;
}

jrtc_router_ctx_t
jrtc_router_get_ctx()
{
    return &g_router_ctx;
}

int
jrtc_router_set_scheduler(struct jrtc_router_ctx* router_ctx, struct jrtc_router_sched_config* sched_config)
{
    int res;

    if (!router_ctx || !sched_config)
        return -1;

    switch (sched_config->sched_policy) {
    case JRTC_ROUTER_NORMAL:
        jrtc_logger(JRTC_INFO, "Using normal scheduling policy\n");
        res = 0;
        break;

    case JRTC_ROUTER_FIFO: {
        int policy = SCHED_FIFO;
        struct sched_param param;
        param.sched_priority = sched_config->sched_priority;
        jrtc_logger(
            JRTC_INFO, "Setting router scheduling policy to SCHED_FIFO, with priority %d\n", param.sched_priority);
        res = pthread_setschedparam(router_ctx->th_ctx.jrtc_router_thread_id, policy, &param);
        break;
    }

    case JRTC_ROUTER_DEADLINE: {
        struct sched_attr attr;
        attr.size = sizeof(attr);
        attr.sched_flags = 0;
        attr.sched_nice = 0;
        attr.sched_priority = 0;
        attr.sched_policy = SCHED_DEADLINE;
        attr.sched_runtime = sched_config->sched_runtime;
        attr.sched_period = sched_config->sched_period;
        attr.sched_deadline = sched_config->sched_deadline;
        jrtc_logger(
            JRTC_INFO,
            "Setting router scheduling policy to SCHED_DEADLINE, with "
            "runtime %lld, period %lld and deadline %lld\n",
            attr.sched_runtime,
            attr.sched_period,
            attr.sched_deadline);
        res = sched_setattr(0, &attr, 0);
        break;
    }

    default:
        jrtc_logger(JRTC_ERROR, "Unknown router scheduling policy\n");
        res = -2;
        break;
    }

    if (res == 0) {
        jrtc_logger(JRTC_INFO, "Scheduling policy applied successfully\n");
    } else {
        jrtc_logger(JRTC_ERROR, "Error setting the scheduling policy of the router\n");
    }
    return res;
}

int
jrtc_router_set_cpu_affinity(struct jrtc_router_ctx* router_ctx, jrtc_router_afinity_mask_t cpu_mask)
{

    cpu_set_t cpuset;

    if (!router_ctx)
        return -1;

    CPU_ZERO(&cpuset); // Clear the CPU affinity mask

    // Convert the uint64_t mask to a cpu_set_t
    for (int i = 0; i < sizeof(jrtc_router_afinity_mask_t) * 8; i++) {
        if (cpu_mask & ((jrtc_router_afinity_mask_t)1 << i)) {
            CPU_SET(i, &cpuset);
        }
    }

    if (pthread_setaffinity_np(router_ctx->th_ctx.jrtc_router_thread_id, sizeof(cpu_set_t), &cpuset) != 0) {
        jrtc_logger(JRTC_ERROR, "Error setting affinity of router thread\n");
        return -1;
    }

    return 0;
}

///////////////////// Internal functionality of the router //////////////////

dapp_id_t
_jrtc_router_reserve_new_app(jrtc_router_ctx_t ctx)
{

    ck_bitmap_iterator_t iter;
    int i;

    ck_bitmap_iterator_init(&iter, ctx->app_metadata.app_bitmap);

    for (i = 0; i < ck_bitmap_bits(ctx->app_metadata.app_bitmap); i++) {
        if (!ck_bitmap_bts(ctx->app_metadata.app_bitmap, i)) {
            return i;
        }
    }

    return -1;
}

void
_jrtc_router_release_app(jrtc_router_ctx_t ctx, dapp_id_t app_id)
{
    ck_bitmap_reset(ctx->app_metadata.app_bitmap, app_id);
}

//////////////////////////// APP API CALLS ////////////////////////////////////

dapp_router_ctx_t
jrtc_router_register_app(size_t app_queue_size)
{

    jrtc_router_ctx_t router_ctx;
    dapp_router_ctx_t dapp;
    dapp_id_t app_id;

    unsigned int mode = CK_HT_MODE_BYTESTRING;

    router_ctx = jrtc_router_get_ctx();

    if (app_queue_size > JRTC_ROUTER_MAX_APP_QUEUE_SIZE) {
        jrtc_logger(
            JRTC_ERROR,
            "Cannot create queue of size %ld. The max queue size is %d\n",
            app_queue_size,
            JRTC_ROUTER_MAX_APP_QUEUE_SIZE);
        return NULL;
    }

    // First we check if there is any space for the new app
    app_id = _jrtc_router_reserve_new_app(router_ctx);

    if (app_id < 0) {
        jrtc_logger(JRTC_ERROR, "Could not reserve an app id\n");
        return NULL;
    }

    dapp = jbpf_calloc(1, sizeof(struct dapp_router_ctx));

    if (!dapp) {
        goto error;
    }

    dapp->app_id = app_id;

    dapp->ringbuffer = jbpf_calloc(app_queue_size + 1, sizeof(ck_ring_buffer_t));

    if (!dapp->ringbuffer) {
        goto dapp_error;
    }

    dapp->data_entry_pool = jbpf_init_mempool(app_queue_size, sizeof(jrtc_router_data_entry_t), JBPF_MEMPOOL_MP);

    if (!dapp->data_entry_pool) {
        goto dapp_ring_error;
    }

    if (!ck_ht_init(
            &dapp->app_out_channel_list, mode, ht_hash_wrapper, &ht_allocator, JRTC_ROUTER_NUM_APP_CHANNELS, 6602834)) {

        goto dapp_destroy_mempool;
    }

    if (!ck_ht_init(
            &dapp->app_in_channel_list, mode, ht_hash_wrapper, &ht_allocator, JRTC_ROUTER_NUM_APP_CHANNELS, 6602834)) {

        ck_ht_destroy(&dapp->app_out_channel_list);
        goto dapp_destroy_mempool;
    }

    // Initialize the queue of the app
    ck_ring_init(&dapp->ring, round_up_pow_of_two(app_queue_size + 1));

    // Store the app in the registry of the router
    router_ctx->app_metadata.ctx[app_id] = dapp;

    jbpf_io_register_thread();

    jrtc_logger(JRTC_INFO, "Registered app with id %d\n", dapp->app_id);

    return dapp;

dapp_destroy_mempool:
    jbpf_destroy_mempool(dapp->data_entry_pool);
dapp_ring_error:
    jbpf_free(dapp->ringbuffer);
dapp_error:
    jbpf_free(dapp);
error:
    _jrtc_router_release_app(router_ctx, app_id);
    return NULL;
}

void
jrtc_router_deregister_app(dapp_router_ctx_t app_ctx)
{

    dapp_id_t app_id;
    jrtc_router_ctx_t router_ctx;
    ck_ht_iterator_t iterator = CK_HT_ITERATOR_INITIALIZER;
    ck_ht_entry_t* cursor;

    if (!app_ctx) {
        return;
    }

    router_ctx = jrtc_router_get_ctx();
    app_id = app_ctx->app_id;

    // Destroy all channels created for this app
    while (ck_ht_next(&app_ctx->app_out_channel_list, &iterator, &cursor) == true) {
        struct dapp_channel_ctx* dapp_channel = ck_ht_entry_value(cursor);
        jbpf_io_destroy_channel(router_ctx->io_ctx, dapp_channel->io_channel);
        jbpf_free(dapp_channel);
    }

    ck_ht_destroy(&app_ctx->app_out_channel_list);

    ck_ht_iterator_init(&iterator);
    while (ck_ht_next(&app_ctx->app_in_channel_list, &iterator, &cursor) == true) {
        struct dapp_channel_ctx* dapp_channel = ck_ht_entry_value(cursor);
        jbpf_io_destroy_channel(router_ctx->io_ctx, dapp_channel->io_channel);
        jbpf_free(dapp_channel);
    }

    ck_ht_destroy(&app_ctx->app_in_channel_list);

    jbpf_free(app_ctx->ringbuffer);
    jbpf_destroy_mempool(app_ctx->data_entry_pool);
    jbpf_free(app_ctx);

    router_ctx->app_metadata.ctx[app_id] = NULL;

    _jrtc_router_release_app(router_ctx, app_id);
}

int
jrtc_router_channel_register_req(
    dapp_router_ctx_t app_ctx, int fwd_dst, int device_id, const char* stream_path, const char* stream_name)
{

    jrtc_router_stream_id_t stream_id_req;
    jrtc_router_generate_stream_id(&stream_id_req, fwd_dst, device_id, stream_path, stream_name);
    return jrtc_router_channel_register_stream_id_req(app_ctx, stream_id_req);
}

int
jrtc_router_channel_register_stream_id_req(dapp_router_ctx_t app_ctx, struct jrtc_router_stream_id stream_id)
{
    jrtc_router_ctx_t router_ctx;
    ck_ht_hash_t h_req;
    ck_ht_entry_t req_table_entry, req_entry;
    int app_id;
    unsigned int bytes;
    jrtc_router_req_entry_t* entry;
    int res = 1;

    if (!app_ctx || app_ctx->app_id < 0 || app_ctx->app_id >= JRTC_ROUTER_MAX_NUM_APPS) {
        return -1;
    }

    app_id = app_ctx->app_id;

    router_ctx = jrtc_router_get_ctx();

    ck_spinlock_lock(&router_ctx->req_table.lock);
    // For each table, check if the corresponding key exists. If it does, just get
    // the value (bitmap) and add one more bit. If it doesn't, then create a new
    // bitmap with the value. We use a spinlock for the updates, so it should be
    // safe to do so.
    ck_ht_hash(&h_req, &router_ctx->req_table.reqs, &stream_id, JRTC_ROUTER_STREAM_ID_BYTE_LEN);
    ck_ht_entry_key_set(&req_table_entry, &stream_id, JRTC_ROUTER_STREAM_ID_BYTE_LEN);

    if (ck_ht_get_spmc(&router_ctx->req_table.reqs, h_req, &req_table_entry) == false) {
        // Add new entry
        entry = jbpf_malloc(sizeof(jrtc_router_req_entry_t));
        if (!entry) {
            res = -1;
            goto out;
        }

        entry->stream_id = stream_id;

        bytes = ck_bitmap_size(JRTC_ROUTER_MAX_NUM_APPS);

        entry->bitmap = jbpf_malloc(bytes);

        if (!entry->bitmap) {
            res = -1;
            jbpf_free(entry);
            goto out;
        }

        ck_bitmap_init(entry->bitmap, JRTC_ROUTER_MAX_NUM_APPS, false);
        ck_bitmap_set(entry->bitmap, app_id);

        ck_ht_entry_set(&req_entry, h_req, &entry->stream_id, JRTC_ROUTER_STREAM_ID_BYTE_LEN, entry);
        if (!ck_ht_set_spmc(&router_ctx->req_table.reqs, h_req, &req_entry)) {
            res = -1;
        } else {
            jrtc_print_stream_id("Added stream id %s\n", &entry->stream_id);
        }

    } else {
        jrtc_logger(JRTC_INFO, "Request already existed. Updating registered apps\n");
        // Just update bitmap of apps, since the request already exists
        entry = ck_ht_entry_value(&req_table_entry);
        ck_bitmap_set(entry->bitmap, app_id);
    }

out:
    ck_spinlock_unlock(&router_ctx->req_table.lock);
    return res;
}

void
jrtc_router_channel_deregister_req(
    dapp_router_ctx_t app_ctx, int fwd_dst, int device_id, const char* stream_path, const char* stream_name)
{

    jrtc_router_stream_id_t stream_id_req;
    jrtc_router_generate_stream_id(&stream_id_req, fwd_dst, device_id, stream_path, stream_name);
    jrtc_router_channel_deregister_stream_id_req(app_ctx, stream_id_req);
}

void
jrtc_router_channel_deregister_stream_id_req(dapp_router_ctx_t app_ctx, struct jrtc_router_stream_id stream_id)
{

    jrtc_router_ctx_t router_ctx;
    ck_ht_hash_t h_req;
    ck_ht_entry_t req_table_entry;
    int app_id;
    jrtc_router_req_entry_t* req_entry;

    if (!app_ctx || app_ctx->app_id < 0 || app_ctx->app_id >= JRTC_ROUTER_MAX_NUM_APPS) {
        return;
    }

    app_id = app_ctx->app_id;

    router_ctx = jrtc_router_get_ctx();

    ck_spinlock_lock(&router_ctx->req_table.lock);

    ck_ht_hash(&h_req, &router_ctx->req_table.reqs, &stream_id, JRTC_ROUTER_STREAM_ID_BYTE_LEN);
    ck_ht_entry_key_set(&req_table_entry, &stream_id, JRTC_ROUTER_STREAM_ID_BYTE_LEN);

    if (ck_ht_get_spmc(&router_ctx->req_table.reqs, h_req, &req_table_entry) == false) {
        goto out;
    } else {
        // Remove the app from the request and if the request has no registered app,
        // delete it
        req_entry = ck_ht_entry_value(&req_table_entry);
        ck_bitmap_reset(req_entry->bitmap, app_id);
        if (ck_bitmap_empty(req_entry->bitmap, JRTC_ROUTER_MAX_NUM_APPS)) {
            ck_epoch_begin(&router_ctx->req_table.app_epoch_record[app_id], NULL);
            ck_ht_remove_spmc(&router_ctx->req_table.reqs, h_req, &req_table_entry);
            ck_epoch_end(&router_ctx->req_table.app_epoch_record[app_id], NULL);

            ck_epoch_call(
                &router_ctx->req_table.app_epoch_record[app_id], &req_entry->epoch_entry, _stream_id_req_destructor);
            ck_epoch_barrier(&router_ctx->req_table.app_epoch_record[app_id]);
        }
    }

out:
    ck_spinlock_unlock(&router_ctx->req_table.lock);
}

int
jrtc_router_receive(dapp_router_ctx_t app_ctx, jrtc_router_data_entry_t* data_entries, size_t num_entries)
{

    int entries_added = 0;
    ck_ht_iterator_t iterator = CK_HT_ITERATOR_INITIALIZER;
    ck_ht_entry_t* cursor;

    jrtc_router_data_entry_t* de;

    while (entries_added < num_entries && ck_ring_dequeue_spsc(&app_ctx->ring, app_ctx->ringbuffer, &de)) {
        data_entries[entries_added] = *de;
        entries_added++;
        jbpf_mbuf_free_from_data_ptr(de, false);
    }

    // Also check input channels
    while (entries_added < num_entries && ck_ht_next(&app_ctx->app_in_channel_list, &iterator, &cursor) == true) {
        struct dapp_channel_ctx* dapp_channel = ck_ht_entry_value(cursor);

        jbpf_channel_buf_ptr data_ptrs[JRTC_ROUTER_DATA_BATCH_SIZE];

        int pending_reqs = num_entries - entries_added;
        int nreqs = JRTC_ROUTER_DATA_BATCH_SIZE > pending_reqs ? pending_reqs : JRTC_ROUTER_DATA_BATCH_SIZE;

        int in_entries = jbpf_io_channel_recv_data(dapp_channel->io_channel, data_ptrs, nreqs);

        for (int entries_idx = 0; entries_idx < in_entries; entries_idx++) {
            data_entries[entries_added].data = data_ptrs[entries_idx];
            data_entries[entries_added].stream_id = dapp_channel->stream_id;
            entries_added++;
        }
    }

    return entries_added;
}

int
jrtc_router_channel_send_output(dapp_channel_ctx_t dapp_chan_ctx)
{

    if (!dapp_chan_ctx) {
        return -1;
    }

    return jbpf_io_channel_submit_buf(dapp_chan_ctx->io_channel);
}

int
jrtc_router_channel_send_output_msg(dapp_channel_ctx_t dapp_chan_ctx, void* data, size_t data_len)
{
    if (!dapp_chan_ctx) {
        return -1;
    }
    if (!data) {
        return -1;
    }
    if (data_len == 0) {
        return -1;
    }

    jbpf_channel_buf_ptr data_buf = jbpf_io_channel_reserve_buf(dapp_chan_ctx->io_channel);
    if (!data_buf) {
        return -1;
    }
    memcpy(data_buf, data, data_len);
    return jbpf_io_channel_submit_buf(dapp_chan_ctx->io_channel);
}

int
jrtc_router_channel_send_input_msg(struct jrtc_router_stream_id stream_id, void* data, size_t data_len)
{

    jrtc_router_ctx_t router_ctx;
    struct jbpf_io_stream_id* sid;

    router_ctx = jrtc_router_get_ctx();
    sid = (struct jbpf_io_stream_id*)&stream_id;
    return jbpf_io_channel_send_msg(router_ctx->io_ctx, sid, data, data_len);
}

void*
jrtc_router_channel_reserve_buf(dapp_channel_ctx_t dapp_chan_ctx)
{

    if (!dapp_chan_ctx) {
        return NULL;
    }

    return jbpf_io_channel_reserve_buf(dapp_chan_ctx->io_channel);
}

void
jrtc_router_channel_release_buf(void* ptr)
{
    if (!ptr) {
        return;
    }

    jbpf_io_channel_release_buf(ptr);
}

dapp_channel_ctx_t
jrtc_router_channel_create(
    dapp_router_ctx_t app_ctx,
    bool is_output,
    int num_elems,
    int elem_size,
    jrtc_router_stream_id_t stream_id,
    char* descriptor,
    size_t descriptor_size)
{
    int direction;
    struct jbpf_io_stream_id* sid;
    jrtc_router_ctx_t router_ctx;
    ck_ht_hash_t h_req;
    ck_ht_entry_t channel_entry;

    if (!app_ctx) {
        jrtc_logger(JRTC_ERROR, "Application context is null.\n");
        goto error;
    }

    direction = is_output ? JBPF_IO_CHANNEL_OUTPUT : JBPF_IO_CHANNEL_INPUT;

    router_ctx = jrtc_router_get_ctx();

    if (!router_ctx) {
        jrtc_logger(JRTC_ERROR, "Failed to get router context.\n");
        goto error;
    }

    struct dapp_channel_ctx* channel;
    channel = jbpf_malloc(sizeof(struct dapp_channel_ctx));
    if (!channel) {
        jrtc_logger(JRTC_ERROR, "Error allocating memory for channel context\n");
        goto error;
    }

    channel->stream_id = stream_id;
    sid = (struct jbpf_io_stream_id*)&channel->stream_id;

    channel->app_ctx = app_ctx;

    jrtc_logger(
        JRTC_INFO,
        "Creating %s channel for application %d with stream id %s\n",
        is_output ? "output" : "input",
        app_ctx->app_id,
        &stream_id);
    channel->io_channel = jbpf_io_create_channel(
        router_ctx->io_ctx, direction, JBPF_IO_CHANNEL_QUEUE, num_elems, elem_size, *sid, descriptor, descriptor_size);

    channel->is_output = is_output;

    if (!channel->io_channel) {
        jrtc_logger(JRTC_ERROR, "Error creating IO channel for application %d\n", app_ctx->app_id);
        goto alloc_error;
    }

    if (is_output) {
        ck_ht_hash(&h_req, &app_ctx->app_out_channel_list, &stream_id, JRTC_ROUTER_STREAM_ID_BYTE_LEN);
        ck_ht_entry_set(&channel_entry, h_req, &stream_id, JRTC_ROUTER_STREAM_ID_BYTE_LEN, channel);

        if (!ck_ht_set_spmc(&app_ctx->app_out_channel_list, h_req, &channel_entry)) {
            jrtc_logger(JRTC_ERROR, "Error adding output channel to application %d\n", app_ctx->app_id);
            goto channel_error;
        } else {
            jrtc_logger(JRTC_INFO, "Added channel to application %d\n", app_ctx->app_id);
        }
    } else {
        ck_ht_hash(&h_req, &app_ctx->app_in_channel_list, &stream_id, JRTC_ROUTER_STREAM_ID_BYTE_LEN);
        ck_ht_entry_set(&channel_entry, h_req, &stream_id, JRTC_ROUTER_STREAM_ID_BYTE_LEN, channel);

        if (!ck_ht_set_spmc(&app_ctx->app_in_channel_list, h_req, &channel_entry)) {
            jrtc_logger(JRTC_ERROR, "Error adding channel to application %d\n", app_ctx->app_id);
            goto channel_error;
        } else {
            jrtc_logger(JRTC_INFO, "Added channel to application %d\n", app_ctx->app_id);
        }
    }

    // Must register channel to application context

    return channel;

channel_error:
    jbpf_io_destroy_channel(router_ctx->io_ctx, channel->io_channel);
alloc_error:
    jbpf_free(channel);
error:
    return NULL;
}

void
jrtc_router_channel_destroy(dapp_channel_ctx_t dapp_chan_ctx)
{

    jrtc_router_ctx_t router_ctx;
    ck_ht_hash_t h_req;
    ck_ht_entry_t channel_entry;

    if (!dapp_chan_ctx) {
        return;
    }

    router_ctx = jrtc_router_get_ctx();

    if (dapp_chan_ctx->is_output) {
        ck_ht_hash(
            &h_req,
            &dapp_chan_ctx->app_ctx->app_out_channel_list,
            &dapp_chan_ctx->stream_id,
            JRTC_ROUTER_STREAM_ID_BYTE_LEN);
        ck_ht_entry_key_set(&channel_entry, &dapp_chan_ctx->stream_id, JRTC_ROUTER_STREAM_ID_BYTE_LEN);

        if (ck_ht_remove_spmc(&dapp_chan_ctx->app_ctx->app_out_channel_list, h_req, &channel_entry)) {
            jbpf_io_destroy_channel(router_ctx->io_ctx, dapp_chan_ctx->io_channel);
            jbpf_free(dapp_chan_ctx);
            jrtc_logger(JRTC_INFO, "Channel found and destroyed successfully\n");
        }
    } else {
        ck_ht_hash(
            &h_req,
            &dapp_chan_ctx->app_ctx->app_in_channel_list,
            &dapp_chan_ctx->stream_id,
            JRTC_ROUTER_STREAM_ID_BYTE_LEN);
        ck_ht_entry_key_set(&channel_entry, &dapp_chan_ctx->stream_id, JRTC_ROUTER_STREAM_ID_BYTE_LEN);

        if (ck_ht_remove_spmc(&dapp_chan_ctx->app_ctx->app_in_channel_list, h_req, &channel_entry)) {
            jbpf_io_destroy_channel(router_ctx->io_ctx, dapp_chan_ctx->io_channel);
            jbpf_free(dapp_chan_ctx);
            jrtc_logger(JRTC_INFO, "Channel found and destroyed successfully\n");
        }
    }
}

int
jrtc_router_input_channel_exists(struct jrtc_router_stream_id stream_id)
{
    jrtc_router_ctx_t router_ctx;

    router_ctx = jrtc_router_get_ctx();

    if (!router_ctx) {
        jrtc_logger(JRTC_ERROR, "Failed to get router context.\n");
        return 0;
    }
    if (!router_ctx->io_ctx) {
        jrtc_logger(JRTC_ERROR, "IO context is null.\n");
        return 0;
    }
    struct jbpf_io_stream_id _stream_id = *(struct jbpf_io_stream_id*)&stream_id;
    if (jbpf_io_find_channel(router_ctx->io_ctx, _stream_id, false)) {
        return 1;
    }
    return 0;
}

int
jrtc_router_create_serialized_msg(jrtc_router_data_entry_t data_entry, void* buf, size_t buf_len)
{
    jrtc_router_ctx_t router_ctx;

    router_ctx = jrtc_router_get_ctx();

    return jbpf_io_channel_pack_msg(router_ctx->io_ctx, data_entry.data, buf, buf_len);
}

jbpf_channel_buf_ptr
jrtc_router_deserialize_msg(void* in_data, size_t in_data_size, struct jrtc_router_stream_id* stream_id, int* req_id)
{

    jrtc_router_ctx_t router_ctx;
    struct jbpf_io_stream_id* sid = (struct jbpf_io_stream_id*)stream_id;

    router_ctx = jrtc_router_get_ctx();

    return jbpf_io_channel_unpack_msg(router_ctx->io_ctx, in_data, in_data_size, sid);
}
