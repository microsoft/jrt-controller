// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "jrtc_yaml.h"

int
parse_yaml_config(const char* filename, config_t* config)
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
    int in_jbpf_io_config = 0;

    if (!yaml_parser_initialize(&parser)) {
        fprintf(stderr, "Failed to initialize YAML parser\n");
        fclose(file);
        return -1;
    }
    yaml_parser_set_input_file(&parser, file);

    while (1) {
        if (!yaml_parser_parse(&parser, &event)) {
            fprintf(stderr, "YAML parsing error: %s\n", parser.problem);
            yaml_parser_delete(&parser);
            fclose(file);
            return -1; // Return error code for invalid YAML
        }
        if (event.type == YAML_STREAM_END_EVENT) {
            yaml_event_delete(&event);
            break; // Stop parsing
        }
        switch (event.type) {
        case YAML_SCALAR_EVENT:
            if (strcmp((char*)event.data.scalar.value, "jrtc_router_config") == 0) {
                in_jrtc_router_config = 1;
            } else if (strcmp((char*)event.data.scalar.value, "thread_config") == 0 && in_jrtc_router_config) {
                in_thread_config = 1;
            } else if (strcmp((char*)event.data.scalar.value, "jbpf_io_config") == 0) {
                in_jbpf_io_config = 1;
            } else if (in_thread_config) {
                if (key[0] == '\0') {
                    strncpy(key, (char*)event.data.scalar.value, sizeof(key) - 1);
                } else {
                    if (strcmp(key, "affinity_mask") == 0) {
                        config->jrtc_router_config.thread_config.affinity_mask = atoi((char*)event.data.scalar.value);
                    } else if (strcmp(key, "hash_sched_config") == 0) {
                        config->jrtc_router_config.thread_config.hash_sched_config =
                            (strcmp((char*)event.data.scalar.value, "true") == 0) ? 1 : 0;
                    }
                    key[0] = '\0';
                }
            } else if (in_jbpf_io_config) {
                if (key[0] == '\0') {
                    strncpy(key, (char*)event.data.scalar.value, sizeof(key) - 1);
                } else {
                    if (strcmp(key, "jbpf_namespace") == 0) {
                        strncpy(
                            config->jbpf_io_config.jbpf_namespace,
                            (char*)event.data.scalar.value,
                            sizeof(config->jbpf_io_config.jbpf_namespace) - 1);
                    } else if (strcmp(key, "jbpf_path") == 0) {
                        strncpy(
                            config->jbpf_io_config.jbpf_path,
                            (char*)event.data.scalar.value,
                            sizeof(config->jbpf_io_config.jbpf_path) - 1);
                    }
                    key[0] = '\0';
                }
            }
            break;
        case YAML_MAPPING_END_EVENT:
            if (in_thread_config) {
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
        if (event.type != YAML_NO_EVENT) {
            yaml_event_delete(&event);
        }
    }

    yaml_parser_delete(&parser);
    fclose(file);
    return 0;
}
