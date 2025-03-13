// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include "stdbool.h"
#include "jbpf_common.h"
#include "jrtc_logging.h"
#include "jrtc_router.h"
#include "jbpf_io_defs.h"
#include "jrtc_config_int.h"
#include "jrtc_config.h"
#include <yaml.h>

char*
expand_env_vars(const char* input)
{
    if (!input)
        return NULL;

    size_t len = strlen(input);
    char* expanded = malloc(len + 1);
    if (!expanded)
        return NULL;

    strcpy(expanded, input);

    regex_t regex;
    regmatch_t match[2];

    // Regex pattern to detect ${VAR_NAME}
    if (regcomp(&regex, "\\$\\{([^}]+)\\}", REG_EXTENDED) != 0) {
        free(expanded);
        return NULL;
    }

    while (regexec(&regex, expanded, 2, match, 0) == 0) {
        char var_name[256] = {0};
        size_t var_len = match[1].rm_eo - match[1].rm_so;
        strncpy(var_name, expanded + match[1].rm_so, var_len);

        char* env_value = getenv(var_name);
        if (!env_value)
            env_value = ""; // Default to empty string if not found

        size_t prefix_len = match[0].rm_so;

        size_t suffix_len = strlen(expanded + match[0].rm_eo);
        size_t new_len = prefix_len + strlen(env_value) + suffix_len;

        char* new_expanded = malloc(new_len + 1);
        if (!new_expanded) {
            free(expanded);
            regfree(&regex);
            return NULL;
        }

        snprintf(
            new_expanded, new_len + 1, "%.*s%s%s", (int)prefix_len, expanded, env_value, expanded + match[0].rm_eo);

        free(expanded);
        expanded = new_expanded;
    }

    regfree(&regex);
    return expanded;
}

void
init_jrtc_config(jrtc_config_t* config)
{
    memset(config, 0, sizeof(jrtc_config_t));

    config->jbpf_io_config.type = JBPF_IO_IPC_PRIMARY;
    config->jbpf_io_config.ipc_config.mem_cfg.memory_size = 1024 * 1024 * 1024;
    config->jrtc_router_config.thread_config.affinity_mask = 1 << 1;
    config->jrtc_router_config.thread_config.has_affinity_mask = false;
    config->jrtc_router_config.thread_config.has_sched_config = false;
    config->jrtc_router_config.thread_config.sched_config.sched_policy = 0;
    config->jrtc_router_config.thread_config.sched_config.sched_priority = 99;
    config->jrtc_router_config.thread_config.sched_config.sched_deadline = 30 * 1000 * 1000;
    config->jrtc_router_config.thread_config.sched_config.sched_runtime = 10 * 1000 * 1000;
    config->jrtc_router_config.thread_config.sched_config.sched_period = 30 * 1000 * 1000;

    strncpy(config->jbpf_io_config.jbpf_path, JBPF_DEFAULT_RUN_PATH, JBPF_RUN_PATH_LEN - 1);
    config->jbpf_io_config.jbpf_path[JBPF_RUN_PATH_LEN - 1] = '\0';
    strncpy(config->jbpf_io_config.jbpf_namespace, JBPF_DEFAULT_NAMESPACE, JBPF_NAMESPACE_LEN - 1);
    config->jbpf_io_config.jbpf_namespace[JBPF_NAMESPACE_LEN - 1] = '\0';

    strncpy(config->jrtc_router_config.io_config.ipc_name, DEFAULT_JRTC_NAME, JBPF_IO_IPC_MAX_NAMELEN - 1);
    config->jrtc_router_config.io_config.ipc_name[JBPF_IO_IPC_MAX_NAMELEN - 1] = '\0';
    strncpy(config->jbpf_io_config.ipc_config.addr.jbpf_io_ipc_name, DEFAULT_JRTC_NAME, JBPF_IO_IPC_MAX_NAMELEN - 1);
    config->jbpf_io_config.ipc_config.addr.jbpf_io_ipc_name[JBPF_IO_IPC_MAX_NAMELEN - 1] = '\0';
}

int
set_config_values(const char* filename, jrtc_config_t* config)
{
    init_jrtc_config(config);

    if (!filename) {
        jrtc_logger(JRTC_INFO, "set_config_values: Filename is NULL\n");
        return 0;
    }
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open YAML file");
        return -1;
    }

    yaml_parser_t parser;
    yaml_event_t event;
    char key[256] = {0};
    int in_jrtc_router_config = 0;
    int in_thread_config = 0;
    int in_sched_config = 0;
    int in_jbpf_io_config = 0;

    if (!yaml_parser_initialize(&parser)) {
        fprintf(stderr, "Failed to initialize YAML parser\n");
        fclose(file);
        return -1;
    }
    yaml_parser_set_input_file(&parser, file);

    while (1) {
        if (!yaml_parser_parse(&parser, &event)) {
            fprintf(stderr, "Failed to parse YAML file: %s\n", parser.problem);
            yaml_parser_delete(&parser);
            fclose(file);
            return -1;
        }
        if (event.type == YAML_STREAM_END_EVENT) {
            yaml_event_delete(&event);
            break;
        }

        switch (event.type) {
        case YAML_SCALAR_EVENT:
            if (key[0] == '\0') {
                // First scalar is a key
                strncpy(key, (char*)event.data.scalar.value, sizeof(key) - 1);
            } else {
                // Second scalar is a value
                char* expanded_value = expand_env_vars((char*)event.data.scalar.value);

                if (in_thread_config && !in_sched_config) {
                    if (strcmp(key, "affinity_mask") == 0) {
                        config->jrtc_router_config.thread_config.affinity_mask = atoi(expanded_value);
                    } else if (strcmp(key, "has_sched_config") == 0) {
                        config->jrtc_router_config.thread_config.has_sched_config =
                            (strcmp(expanded_value, "true") == 0) ? 1 : 0;
                    }
                } else if (in_sched_config) {
                    if (strcmp(key, "sched_policy") == 0) {
                        config->jrtc_router_config.thread_config.sched_config.sched_policy = atoi(expanded_value);
                    } else if (strcmp(key, "sched_priority") == 0) {
                        config->jrtc_router_config.thread_config.sched_config.sched_priority = atoi(expanded_value);
                    } else if (strcmp(key, "sched_deadline") == 0) {
                        config->jrtc_router_config.thread_config.sched_config.sched_deadline = atoll(expanded_value);
                    } else if (strcmp(key, "sched_runtime") == 0) {
                        config->jrtc_router_config.thread_config.sched_config.sched_runtime = atoll(expanded_value);
                    } else if (strcmp(key, "sched_period") == 0) {
                        config->jrtc_router_config.thread_config.sched_config.sched_period = atoll(expanded_value);
                    }
                } else if (in_jbpf_io_config) {
                    if (strcmp(key, "jbpf_namespace") == 0) {
                        strncpy(
                            config->jbpf_io_config.jbpf_namespace,
                            expanded_value,
                            sizeof(config->jbpf_io_config.jbpf_namespace) - 1);
                    } else if (strcmp(key, "jbpf_path") == 0) {
                        strncpy(
                            config->jbpf_io_config.jbpf_path,
                            expanded_value,
                            sizeof(config->jbpf_io_config.jbpf_path) - 1);
                    }
                } else if (in_jrtc_router_config) {
                    if (strcmp(key, "ipc_name") == 0) {
                        strncpy(
                            config->jrtc_router_config.io_config.ipc_name,
                            expanded_value,
                            sizeof(config->jrtc_router_config.io_config.ipc_name) - 1);
                        strncpy(
                            config->jbpf_io_config.ipc_config.addr.jbpf_io_ipc_name,
                            expanded_value,
                            sizeof(config->jbpf_io_config.ipc_config.addr.jbpf_io_ipc_name) - 1);
                    }
                }

                free(expanded_value);
                key[0] = '\0'; // Reset key for next key-value pair
            }
            break;

        case YAML_MAPPING_START_EVENT:
            if (strcmp(key, "jrtc_router_config") == 0) {
                in_jrtc_router_config = 1;
            } else if (strcmp(key, "thread_config") == 0 && in_jrtc_router_config) {
                in_thread_config = 1;
            } else if (strcmp(key, "sched_config") == 0 && in_thread_config) {
                in_sched_config = 1;
            } else if (strcmp(key, "jbpf_io_config") == 0) {
                in_jbpf_io_config = 1;
            }
            key[0] = '\0'; // Reset key
            break;

        case YAML_MAPPING_END_EVENT:
            if (in_sched_config) {
                in_sched_config = 0;
            } else if (in_thread_config) {
                in_thread_config = 0;
            } else if (in_jrtc_router_config) {
                in_jrtc_router_config = 0;
            } else if (in_jbpf_io_config) {
                in_jbpf_io_config = 0;
            }
            break;

        default:
            break;
        }

        yaml_event_delete(&event);
    }

    yaml_parser_delete(&parser);
    fclose(file);
    return 0;
}
