// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "jrtc_yaml.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>

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

int
parse_yaml_config(const char* filename, yaml_config_t* config)
{
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
