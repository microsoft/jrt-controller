// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#ifndef JRTC_H
#define JRTC_H

#include <stdbool.h>
#include <stdatomic.h>
#include "jrtc_sched.h"
#include "jrtc_router_app_api.h"

/**
 * @brief Max app name size
 * @ingroup controller
 */
#define MAX_APP_NAME_SIZE 16
#define MAX_APP_PARAMS 255

typedef pthread_t app_id_t;

/**
 * @brief The jrtc_app_env struct
 * @ingroup controller
 * The application environment
 * app_name: The application name
 * app_tid: The application thread id
 * app_id: The application id
 * app_handle: The application handle
 * dapp_ctx: The router context
 * io_queue_size: The io queue size
 * app_exit: The application exit flag
 * sched_config: The scheduling configuration
 */
struct jrtc_app_env
{
    char* app_name;
    pthread_t app_tid;
    pthread_t app_id;
    void* app_handle;
    dapp_router_ctx_t dapp_ctx;
    uint32_t io_queue_size;
    atomic_bool app_exit;
    jrtc_sched_config_t sched_config;
    char* app_params[MAX_APP_PARAMS];
};

#endif