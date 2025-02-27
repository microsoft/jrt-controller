// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#ifndef JRTC_SCHED_H
#define JRTC_SCHED_H

#include <pthread.h>
#include <stdint.h>
#include <linux/types.h>

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

/**
 * @brief The jrtc_sched_policy_e enum
 * @ingroup controller
 * The scheduling policy
 * JRTC_SCHED_NORMAL: Normal scheduling
 * JRTC_SCHED_FIFO: First in, first out scheduling
 * JRTC_SCHED_DEADLINE: Deadline scheduling
 */
typedef enum
{
    JRTC_SCHED_NORMAL = 0,
    JRTC_SCHED_FIFO,
    JRTC_SCHED_DEADLINE,
} jrtc_sched_policy_e;

/**
 * @brief The jrtc_sched_config struct
 * @ingroup controller
 * The scheduling configuration
 * sched_policy: The scheduling policy
 * sched_priority: The scheduling priority
 * sched_runtime_us: The runtime in microseconds
 * sched_deadline_us: The deadline in microseconds
 * sched_period_us: The period in microseconds
 */
typedef struct jrtc_sched_config
{
    jrtc_sched_policy_e sched_policy;
    int sched_priority;
    uint64_t sched_runtime_us;
    uint64_t sched_deadline_us;
    uint64_t sched_period_us;
} jrtc_sched_config_t;

/**
 * @brief Set the scheduler
 * @ingroup controller
 * @param tid The thread id
 * @param sched_config The scheduling configuration
 * @return 0 on success, -1 on failure
 */
int
jrtc_thread_set_scheduler(pthread_t tid, struct jrtc_sched_config* sched_config);

#endif