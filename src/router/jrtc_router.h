// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#ifndef JRTC_ROUTER_H
#define JRTC_ROUTER_H

#include <stdbool.h>
#include "jbpf_io_defs.h"

// Forward declaration
struct yaml_config;

/**
 * @brief The jrtc_router_sched_policy_e enum
 * @ingroup router
 * The scheduling policy
 * JRTC_ROUTER_NORMAL: Normal scheduling
 * JRTC_ROUTER_FIFO: First in, first out scheduling
 * JRTC_ROUTER_DEADLINE: Deadline scheduling
 */
typedef enum
{
    JRTC_ROUTER_NORMAL = 0,
    JRTC_ROUTER_FIFO,
    JRTC_ROUTER_DEADLINE,
} jrtc_router_sched_policy_e;

typedef uint64_t jrtc_router_afinity_mask_t;

/**
 * @brief The jrtc_router_sched_config struct
 * @ingroup router
 * The scheduling configuration
 * sched_policy: The scheduling policy
 * sched_priority: The scheduling priority
 * sched_runtime: The runtime
 * sched_deadline: The deadline
 * sched_period: The period
 */
struct jrtc_router_sched_config
{
    jrtc_router_sched_policy_e sched_policy;
    int sched_priority;
    uint64_t sched_runtime;
    uint64_t sched_deadline;
    uint64_t sched_period;
};

/**
 * @brief The jrtc_router_thread_config struct
 * @ingroup router
 * The thread configuration
 * has_affinity_mask: The affinity mask
 * affinity_mask: The affinity mask
 * has_sched_config: The scheduling configuration
 */
struct jrtc_router_thread_config
{
    bool has_affinity_mask;
    jrtc_router_afinity_mask_t affinity_mask;
    bool has_sched_config;
    struct jrtc_router_sched_config sched_config;
};

/**
 * @brief The jrtc_router_io_config struct
 * @ingroup router
 * The io configuration
 * ipc_name: The ipc name
 */
struct jrtc_router_io_config
{
    char ipc_name[32];
};

/**
 * @brief The jrtc_router_config struct
 * @ingroup router
 * The router configuration
 * thread_config: The thread configuration
 * io_config: The io configuration
 */
struct jrtc_router_config
{
    struct jrtc_router_thread_config thread_config;
    struct jrtc_router_io_config io_config;
};

typedef struct jrtc_router_ctx* jrtc_router_ctx_t;

/**
 * @brief Initialize the router
 * @ingroup router
 * @param config The configuration
 * @return 0 on success, -1 on failure
 */
int
jrtc_router_init(struct yaml_config* config);

/**
 * @brief Set the scheduler
 * @ingroup router
 * @param router_ctx The router context
 * @param sched_config The scheduling configuration
 * @return 0 on success, -1 on failure
 */
int
jrtc_router_set_scheduler(struct jrtc_router_ctx* router_ctx, struct jrtc_router_sched_config* sched_config);

/**
 * @brief Set the cpu affinity
 * @ingroup router
 * @param router_ctx The router context
 * @param cpu_mask The cpu mask
 * @return 0 on success, -1 on failure
 */
int
jrtc_router_set_cpu_affinity(struct jrtc_router_ctx* router_ctx, jrtc_router_afinity_mask_t cpu_mask);

/**
 * @brief Stop the router
 * @ingroup router
 * @return 0 on success, -1 on failure
 */
int
jrtc_router_stop();

/**
 * @brief Get the router context
 * @ingroup router
 * @return The router context
 */
jrtc_router_ctx_t
jrtc_router_get_ctx();

#endif