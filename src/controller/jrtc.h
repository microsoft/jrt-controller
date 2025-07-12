// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#ifndef JRTC_H
#define JRTC_H

// For atomic types
#ifdef __cplusplus
// C++ atomic type alias
#include <atomic> // C++: std::atomic
// Alias atomic_bool to std::atomic<bool> in C++
using atomic_bool = std::atomic<bool>;
#else
// C atomic type alias
#include <stdatomic.h> // C: atomic_bool
#endif

#include <stdbool.h>
#include "jrtc_sched.h"
#include "jrtc_router_app_api.h"

/**
 * @brief Max app name size
 * @ingroup controller
 */
#define MAX_APP_NAME_SIZE 16
#define MAX_APP_PARAMS 255
#define MAX_DEVICE_MAPPING 255
#define MAX_APP_MODULES 255

typedef pthread_t app_id_t;

typedef struct _key_value_pair
{
    char* key;
    char* val;
} key_value_pair_t;

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
 * app_path: The application path
 * params: The application parameters
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
    char* app_path;
    key_value_pair_t params[MAX_APP_PARAMS];
    key_value_pair_t device_mapping[MAX_DEVICE_MAPPING];
    char* app_modules[MAX_APP_MODULES];
    void* shared_python_state; // Pointer to shared Python state for multi-threaded apps
};

#endif