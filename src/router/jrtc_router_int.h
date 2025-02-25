// Copyright (c) Microsoft Corporation. All rights reserved.
#ifndef JRTC_ROUTER_INT_H
#define JRTC_ROUTER_INT_H

#define _GNU_SOURCE
#include <unistd.h>
#include <linux/unistd.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <sys/syscall.h>
#include <pthread.h>

#include "ck_ring.h"
#include "ck_ht.h"
#include "ck_bitmap.h"
#include "ck_spinlock.h"
#include "ck_epoch.h"

#include "jbpf_mempool.h"
#include "jrtc_router_app_api.h"
#include "jrtc_logging.h"

#define gettid() syscall(__NR_gettid)

#define SCHED_DEADLINE 6

///// SCHED_DEADLINE defs /////

#ifdef __x86_64__
#define __NR_sched_setattr 314
#define __NR_sched_getattr 315
#endif

#ifdef __i386__
#define __NR_sched_setattr 351
#define __NR_sched_getattr 352
#endif

#ifdef __arm__
#define __NR_sched_setattr 380
#define __NR_sched_getattr 381
#endif

#define jrtc_print_stream_id(msg, X)                                                                         \
    char sname[JRTC_ROUTER_STREAM_ID_BYTE_LEN * 3];                                                          \
    _jbpf_io_tohex_str((char*)X, JRTC_ROUTER_STREAM_ID_BYTE_LEN, sname, JRTC_ROUTER_STREAM_ID_BYTE_LEN * 3); \
    jrtc_logger(JRTC_INFO, msg, sname);

struct sched_attr
{
    __u32 size;

    __u32 sched_policy;
    __u64 sched_flags;

    /* SCHED_NORMAL, SCHED_BATCH */
    __s32 sched_nice;

    /* SCHED_FIFO, SCHED_RR */
    __u32 sched_priority;

    /* SCHED_DEADLINE (nsec) */
    __u64 sched_runtime;
    __u64 sched_deadline;
    __u64 sched_period;
};

struct jrtc_router_thread_ctx
{
    pthread_t jrtc_router_thread_id;
};

///// JRTC ROUTER DEFS ////////

#define JRTC_ROUTER_MAX_APP_QUEUE_SIZE (10000)

#define JRTC_ROUTER_MAX_NUM_APPS (128)

#define JRTC_ROUTER_MAX_NUM_DEV_ID (256)
#define JRTC_ROUTER_MAX_NUM_FWD_DST (256)
#define JRTC_ROUTER_MAX_REQ_ENTRIES (2048)

#define JRTC_ROUTER_NUM_REQ_LOOKUPS (16)
#define JRTC_ROUTER_INIT_NUM_REQ_ENTRIES (2048)

#define JRTC_ROUTER_NUM_APP_CHANNELS (16)

#define JRTC_ROUTER_DATA_BATCH_SIZE (16)

typedef int dapp_id_t;

struct dapp_router_ctx
{
    ck_ring_t ring;
    ck_ring_buffer_t* ringbuffer;
    jbpf_mempool_t* data_entry_pool;

    ck_ht_t app_out_channel_list;
    ck_ht_t app_in_channel_list;

    dapp_id_t app_id;
};

typedef struct jrtc_router_req_entry
{
    jrtc_router_stream_id_t stream_id;
    ck_bitmap_t* bitmap;
    ck_epoch_entry_t epoch_entry;
} jrtc_router_req_entry_t;

typedef struct jrtc_router_req_table
{
    ck_ht_t reqs;
    ck_bitmap_t* lookup_result;
    ck_spinlock_t lock;
    ck_epoch_t router_epoch;
    ck_epoch_record_t router_epoch_record;
    ck_epoch_record_t app_epoch_record[JRTC_ROUTER_MAX_NUM_APPS];
} jrtc_router_req_table_t;

typedef struct jrtc_router_app_data
{
    struct dapp_router_ctx* ctx[JRTC_ROUTER_MAX_NUM_APPS];
    ck_bitmap_t* app_bitmap;
} jrtc_router_app_data_t;

struct jrtc_router_ctx
{
    struct jbpf_io_ctx* io_ctx;
    struct jrtc_router_thread_ctx th_ctx;

    // Used for storing all the requests made by the apps
    jrtc_router_req_table_t req_table;

    // Holds all the app metadata
    jrtc_router_app_data_t app_metadata;
};

#endif
