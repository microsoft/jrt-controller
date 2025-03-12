// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
/**
    This test tests the yaml parsing functionality in jrtc_yaml.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "jrtc_router.h"
#include "jrtc_yaml_int.h"
#include "jrtc_yaml.h"

void
test_yaml_parsing()
{
    printf("Running tests for YAML parsing...\n");
    char* jrtc_path = getenv("JRTC_PATH");
    assert(jrtc_path != NULL);
    printf("JRTC_PATH: %s\n", jrtc_path);

    char config_file[256];

    // Test 1: Valid YAML file
    {
        //   jrtc_router_config:
        //     thread_config:
        //       affinity_mask: 3
        //       has_sched_config: true
        //       sched_config:
        //         sched_policy: 0
        //         sched_priority: 99
        //         sched_deadline: 30000000
        //         sched_runtime: 10000000
        //         sched_period: 30000000
        //   jbpf_io_config:
        //     jbpf_namespace: "default"
        //     jbpf_path: "/var/lib/jbpf"
        snprintf(config_file, sizeof(config_file), "%s/jrtc_tests/test_data/yaml/valid.yaml", jrtc_path);
        yaml_config_t config;
        printf("Parsing config file: %s\n", config_file);
        int result = set_config_values(config_file, &config);
        assert(result == 0);
        assert(config.jrtc_router_config.thread_config.affinity_mask == 3);
        assert(config.jrtc_router_config.thread_config.has_sched_config == 1);
        assert(config.jrtc_router_config.thread_config.sched_config.sched_policy == 0);
        assert(config.jrtc_router_config.thread_config.sched_config.sched_deadline == 30000000);
        assert(config.jrtc_router_config.thread_config.sched_config.sched_runtime == 10000000);
        assert(config.jrtc_router_config.thread_config.sched_config.sched_period == 30000000);
        assert(config.jrtc_router_config.thread_config.sched_config.sched_priority == 99);
        assert(strcmp(config.jbpf_io_config.jbpf_namespace, "jrtc") == 0);
        assert(strcmp(config.jbpf_io_config.jbpf_path, "/var/run/jrtc") == 0);
        printf("Test 1 passed: Valid YAML file parsed successfully.\n");
    }

    // Test 2: Invalid YAML file
    {
        snprintf(config_file, sizeof(config_file), "%s/jrtc_tests/test_data/yaml/invalid.yaml", jrtc_path);
        yaml_config_t config;
        printf("Parsing config file: %s\n", config_file);
        int result = set_config_values(config_file, &config);
        assert(result != 0);
        // check the default values
        assert(config.jrtc_router_config.thread_config.affinity_mask == 2);
        assert(config.jrtc_router_config.thread_config.has_sched_config == 0);
        assert(config.jrtc_router_config.thread_config.sched_config.sched_policy == 0);
        assert(config.jrtc_router_config.thread_config.sched_config.sched_deadline == 30000000);
        assert(config.jrtc_router_config.thread_config.sched_config.sched_runtime == 10000000);
        assert(config.jrtc_router_config.thread_config.sched_config.sched_period == 30000000);
        assert(config.jrtc_router_config.thread_config.sched_config.sched_priority == 99);
        assert(strcmp(config.jbpf_io_config.jbpf_namespace, "jbpf") == 0);
        assert(strcmp(config.jbpf_io_config.jbpf_path, "/tmp") == 0);
        printf("Test 2 passed: Invalid YAML file handled correctly.\n");
    }

    // Test 3: Empty YAML file
    {
        snprintf(config_file, sizeof(config_file), "%s/jrtc_tests/test_data/yaml/empty.yaml", jrtc_path);
        yaml_config_t config;
        printf("Parsing config file: %s\n", config_file);
        int result = set_config_values(config_file, &config);
        assert(result == 0);
        // check the default values
        assert(config.jrtc_router_config.thread_config.affinity_mask == 2);
        assert(config.jrtc_router_config.thread_config.has_sched_config == 0);
        assert(config.jrtc_router_config.thread_config.sched_config.sched_policy == 0);
        assert(config.jrtc_router_config.thread_config.sched_config.sched_deadline == 30000000);
        assert(config.jrtc_router_config.thread_config.sched_config.sched_runtime == 10000000);
        assert(config.jrtc_router_config.thread_config.sched_config.sched_period == 30000000);
        assert(config.jrtc_router_config.thread_config.sched_config.sched_priority == 99);
        assert(strcmp(config.jbpf_io_config.jbpf_namespace, "jbpf") == 0);
        assert(strcmp(config.jbpf_io_config.jbpf_path, "/tmp") == 0);        
        printf("Test 3 passed: Empty YAML file handled correctly.\n");
    }

    // Test 4: Valid YAML file but contains only a few parameters
    {
        // jbpf_io_config:
        //   jbpf_namespace: jrtc
        snprintf(config_file, sizeof(config_file), "%s/jrtc_tests/test_data/yaml/valid_incomplete.yaml", jrtc_path);
        yaml_config_t config;
        printf("Parsing config file: %s\n", config_file);
        int result = set_config_values(config_file, &config);
        assert(result == 0);
        assert(strcmp(config.jbpf_io_config.jbpf_namespace, "jrtc") == 0);
        // check the default values
        assert(config.jrtc_router_config.thread_config.affinity_mask == 2);
        assert(config.jrtc_router_config.thread_config.has_sched_config == 0);
        assert(config.jrtc_router_config.thread_config.sched_config.sched_policy == 0);
        assert(config.jrtc_router_config.thread_config.sched_config.sched_deadline == 30000000);
        assert(config.jrtc_router_config.thread_config.sched_config.sched_runtime == 10000000);
        assert(config.jrtc_router_config.thread_config.sched_config.sched_period == 30000000);
        assert(config.jrtc_router_config.thread_config.sched_config.sched_priority == 99);
        assert(strcmp(config.jbpf_io_config.jbpf_path, "/tmp") == 0);        
        printf("Test 4 passed: Valid YAML file (incomplete values) parsed successfully.\n");
    }

    // Test 5: Valid YAML file with env substitution
    {
        // jbpf_io_config:
        //   jbpf_namespace: jrtc_${JRTC_TEST_ID}
        snprintf(config_file, sizeof(config_file), "%s/jrtc_tests/test_data/yaml/valid_env.yaml", jrtc_path);
        setenv("JRTC_TEST_ID", "1234", 1);
        yaml_config_t config;
        printf("Parsing config file: %s\n", config_file);
        int result = set_config_values(config_file, &config);
        assert(result == 0);
        assert(strcmp(config.jbpf_io_config.jbpf_namespace, "jrtc_1234") == 0);
        // check the default values
        assert(config.jrtc_router_config.thread_config.affinity_mask == 2);
        assert(config.jrtc_router_config.thread_config.has_sched_config == 0);
        assert(config.jrtc_router_config.thread_config.sched_config.sched_policy == 0);
        assert(config.jrtc_router_config.thread_config.sched_config.sched_deadline == 30000000);
        assert(config.jrtc_router_config.thread_config.sched_config.sched_runtime == 10000000);
        assert(config.jrtc_router_config.thread_config.sched_config.sched_period == 30000000);
        assert(config.jrtc_router_config.thread_config.sched_config.sched_priority == 99);
        assert(strcmp(config.jbpf_io_config.jbpf_path, "/tmp") == 0);        
        printf("Test 5 passed: Valid YAML file (with env substitution) parsed successfully.\n");
    }

    printf("All tests passed!\n");
}

void
assert_string_equals(const char* input, const char* expected)
{
    char* result = expand_env_vars(input);
    if (result == NULL) {
        printf("Error expanding environment variables\n");
        exit(1);
    }
    if (strcmp(result, expected) != 0) {
        printf("Test failed: Expected '%s', but got '%s'\n", expected, result);
        free(result);
        exit(1);
    }
    free(result);
}

void
test_env_expansions()
{
    // Test 1: Simple environment variable expansion
    setenv("JRTC_UNIT_TEST_1", "xxxx", 1);
    assert_string_equals("${JRTC_UNIT_TEST_1}", "xxxx");

    // Test 2: Expanding multiple environment variables
    setenv("JRTC_UNIT_TEST_2", "/home/xxx", 1);
    assert_string_equals("${JRTC_UNIT_TEST_1} is at ${JRTC_UNIT_TEST_2}", "xxxx is at /home/xxx");

    // Test 3: Non-existent environment variable (should return an empty string)
    assert_string_equals("${NONEXISTENT_VAR}", "");

    // Test 4: Empty input string (should return an empty string)
    assert_string_equals("", "");

    // Test 5: Nested environment variables (should resolve both correctly)
    setenv("AAA", "1111", 1);
    setenv("bbbb", "2222", 1);
    assert_string_equals("/${AAA}/documents/${bbbb}", "/1111/documents/2222");

    // Test 6: Literal `${}` without environment variable (should return empty)
    assert_string_equals("${NOT_DEFINED}", "");
}

int
main()
{
    test_yaml_parsing();
    test_env_expansions();
    return 0;
}
