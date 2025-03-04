// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef JRTC_REST_SERVER_H
#define JRTC_REST_SERVER_H

#include "jrtc.h"

/**
 * @brief The load_app_request struct
 * @ingroup rest_api_lib
 * app: The application
 * app_size: The application size
 * app_name: The application name
 * runtime_us: The runtime in microseconds
 * deadline_us: The deadline in microseconds
 * period_us: The period in microseconds
 * ioq_size: The io queue size
 * app_path: The application path
 * params: The application parameters
 */
typedef struct load_app_request
{
    char* app;
    size_t app_size;
    char* app_name;
    uint32_t runtime_us;
    uint32_t deadline_us;
    uint32_t period_us;
    uint32_t ioq_size;
    char* app_path;
    char* app_type;
    app_param_key_value_pair_t params[MAX_APP_PARAMS];
} load_app_request_t;

/**
 * @brief The jrtc_rest_callbacks struct
 * @ingroup rest_api_lib
 * load_app: The load app callback
 * unload_app: The unload app callback
 */
typedef struct
{
    int (*load_app)(load_app_request_t);
    int (*unload_app)(int);
} jrtc_rest_callbacks;

/**
 * @brief Start the rest server
 * @ingroup rest_api_lib
 * @param ptr The pointer
 * @param port The port
 * @param callbacks The callbacks
 */
extern void
jrtc_start_rest_server(void* ptr, unsigned int port, jrtc_rest_callbacks* callbacks);

/**
 * @brief Stop the rest server
 * @ingroup rest_api_lib
 * @param ptr The pointer
 */
extern void
jrtc_stop_rest_server(void* ptr);

/**
 * @brief Create the rest server
 * @ingroup rest_api_lib
 * @return The rest server
 */
extern void*
jrtc_create_rest_server();

#endif
