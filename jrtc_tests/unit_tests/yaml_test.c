// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
/**
    This test tests the yaml parsing functionality in jrtc_yaml.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "jrtc_yaml.h"

void
test_yaml_parsing()
{
    printf("Running tests for YAML parsing...\n");
    char* jrtc_path = getenv("JRTC_PATH");
    if (jrtc_path == NULL) {
        fprintf(stderr, "JRTC_PATH environment variable is not set.\n");
        return;
    }
    printf("JRTC_PATH: %s\n", jrtc_path);

    char config_file[256];

    // Test 1: Valid YAML file
    {
        // jrtc_router_config:
        //   thread_config:
        //     affinity_mask: 2
        //     hash_sched_config: false
        // jbpf_io_config:
        //   jbpf_namespace: jrtc
        //   jbpf_path: /var/run/jbpf
        snprintf(config_file, sizeof(config_file), "%s/jrtc_tests/test_data/yaml/valid.yaml", jrtc_path);
        config_t config;
        printf("Parsing config file: %s\n", config_file);
        int result = parse_yaml_config(config_file, &config);
        assert(result == 0);
        assert(config.jrtc_router_config.thread_config.affinity_mask == 3);
        assert(config.jrtc_router_config.thread_config.hash_sched_config == 1);
        assert(strcmp(config.jbpf_io_config.jbpf_namespace, "jrtc") == 0);
        assert(strcmp(config.jbpf_io_config.jbpf_path, "/var/run/jrtc") == 0);
        printf("Test 1 passed: Valid YAML file parsed successfully.\n");
    }

    // Test 2: Invalid YAML file
    {
        snprintf(config_file, sizeof(config_file), "%s/jrtc_tests/test_data/yaml/invalid.yaml", jrtc_path);
        config_t config;
        printf("Parsing config file: %s\n", config_file);
        int result = parse_yaml_config(config_file, &config);
        assert(result != 0);
        printf("Test 2 passed: Invalid YAML file handled correctly.\n");
    }

    // Test 3: Empty YAML file
    {
        snprintf(config_file, sizeof(config_file), "%s/jrtc_tests/test_data/yaml/empty.yaml", jrtc_path);
        config_t config;
        printf("Parsing config file: %s\n", config_file);
        int result = parse_yaml_config(config_file, &config);
        assert(result == 0);
        printf("Test 3 passed: Empty YAML file handled correctly.\n");
    }

    // Test 4: Valid YAML file but contains only a few parameters
    {
        // jbpf_io_config:
        //   jbpf_namespace: jrtc
        snprintf(config_file, sizeof(config_file), "%s/jrtc_tests/test_data/yaml/valid.yaml", jrtc_path);
        config_t config;
        printf("Parsing config file: %s\n", config_file);
        int result = parse_yaml_config(config_file, &config);
        assert(result == 0);
        assert(strcmp(config.jbpf_io_config.jbpf_namespace, "jrtc") == 0);
        printf("Test 4 passed: Valid YAML file (incomplete values) parsed successfully.\n");
    }

    printf("All tests passed!\n");
}

int
main()
{
    test_yaml_parsing();
    return 0;
}
