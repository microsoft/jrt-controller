// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#ifndef JRTC_YAML_H
#define JRTC_YAML_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <yaml.h>

#define MAX_NAMESPACE_LENGTH 256
#define MAX_PATH_LENGTH 256

typedef struct
{
    int affinity_mask;
    int hash_sched_config; // Boolean (0 or 1)
} thread_config_t;

typedef struct
{
    thread_config_t thread_config;
} jrtc_router_config_t;

typedef struct
{
    char jbpf_namespace[MAX_NAMESPACE_LENGTH];
    char jbpf_path[MAX_PATH_LENGTH];
} jbpf_io_config_t;

typedef struct
{
    jrtc_router_config_t jrtc_router_config;
    jbpf_io_config_t jbpf_io_config;
} yaml_config_t;

/**
 * @brief Parses a YAML configuration file and fills the provided config structure.
 * @param filename The path to the YAML file.
 * @param config Pointer to the structure to fill with parsed data.
 * @return 0 on success, -1 on failure.
 */
int
parse_yaml_config(const char* filename, yaml_config_t* config);

#endif // JRTC_YAML_H