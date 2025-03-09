// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#ifndef JRTC_YAML_H
#define JRTC_YAML_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <yaml.h>

typedef struct {
    int affinity_mask;
    int hash_sched_config;  // Boolean (0 or 1)
} thread_config_t;

typedef struct {
    thread_config_t thread_config;
} jrtc_router_config_t;

typedef struct {
    char jbpf_namespace[256];
    char jbpf_path[256];
} jbpf_io_config_t;

typedef struct {
    jrtc_router_config_t jrtc_router_config;
    jbpf_io_config_t jbpf_io_config;
} config_t;

int parse_yaml_config(const char *filename, config_t *config);

#endif // JRTC_YAML_H