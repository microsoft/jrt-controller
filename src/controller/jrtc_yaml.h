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
    int sched_policy;
    int sched_priority;
    long long sched_deadline;
    long long sched_runtime;
    long long sched_period;
} sched_config_t;

typedef struct
{
    int affinity_mask;
    int has_affinity_mask; // Boolean (0 or 1)
    int has_sched_config;  // Boolean (0 or 1)
    sched_config_t sched_config;
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
 * @brief Expands environment variables in a given string (e.g., "Path: ${HOME}/test")
 * @param input - The input string with possible ${VAR} placeholders.
 * @return - The expanded string (must be freed by the caller).
 */
char*
expand_env_vars(const char* input);

/**
 * @brief Parses a YAML configuration file and fills the provided config structure.
 * @param filename The path to the YAML file.
 * @param config Pointer to the structure to fill with parsed data.
 * @return 0 on success, -1 on failure.
 */
int
parse_yaml_config(const char* filename, yaml_config_t* config);

#endif // JRTC_YAML_H