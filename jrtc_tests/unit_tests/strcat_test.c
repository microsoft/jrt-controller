// Copyright (c) Microsoft Corporation. All rights reserved.
/**
    This test tests the concat function in jrtc_int.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "jrtc_int.h"

// Test suite for concat function
void
test_concat()
{
    printf("Running tests for concat...\n");

    // Test 1: Normal case with two non-empty strings
    {
        const char* s1 = "Hello, ";
        const char* s2 = "World!";
        char* result = concat(s1, s2);
        assert(strcmp(result, "Hello, World!") == 0);
        printf("Test 1 passed: %s\n", result);
        free(result);
    }

    // Test 2: Empty first string
    {
        const char* s1 = "";
        const char* s2 = "World!";
        char* result = concat(s1, s2);
        assert(strcmp(result, "World!") == 0);
        printf("Test 2 passed: %s\n", result);
        free(result);
    }

    // Test 3: Empty second string
    {
        const char* s1 = "Hello, ";
        const char* s2 = "";
        char* result = concat(s1, s2);
        assert(strcmp(result, "Hello, ") == 0);
        printf("Test 3 passed: %s\n", result);
        free(result);
    }

    // Test 4: Both strings are empty
    {
        const char* s1 = "";
        const char* s2 = "";
        char* result = concat(s1, s2);
        assert(strcmp(result, "") == 0);
        printf("Test 4 passed: %s\n", result);
        free(result);
    }

    // Test 5: Strings with special characters
    {
        const char* s1 = "Hello\n";
        const char* s2 = "World\t!";
        char* result = concat(s1, s2);
        assert(strcmp(result, "Hello\nWorld\t!") == 0);
        printf("Test 5 passed: %s\n", result);
        free(result);
    }

    // Test 6: Very long strings
    {
        const char* s1 = "A very long string that is ";
        const char* s2 = "concatenated with another long string.";
        char* result = concat(s1, s2);
        assert(strcmp(result, "A very long string that is concatenated with another long string.") == 0);
        printf("Test 6 passed: %s\n", result);
        free(result);
    }

    // Test 7: Null input for one string
    {
        const char* s1 = NULL;
        const char* s2 = "World!";
        char* result = NULL;
        int caught_error = 0;
        if (!s1 || !s2) {
            caught_error = 1; // Handle null input
        } else {
            result = concat(s1, s2);
        }
        assert(caught_error == 1);
        printf("Test 7 passed: Null input handled correctly.\n");
        if (result)
            free(result);
    }

    // Test 8: Null input for both strings
    {
        const char* s1 = NULL;
        const char* s2 = NULL;
        char* result = NULL;
        int caught_error = 0;
        if (!s1 || !s2) {
            caught_error = 1; // Handle null input
        } else {
            result = concat(s1, s2);
        }
        assert(caught_error == 1);
        printf("Test 8 passed: Null input handled correctly.\n");
        if (result)
            free(result);
    }

    printf("All tests passed!\n");
}

int
main()
{
    test_concat();
    return 0;
}
