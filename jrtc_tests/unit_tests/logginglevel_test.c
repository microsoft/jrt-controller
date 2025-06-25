// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
/**
    This test tests the logging level functionality in jrtc_logging.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "jrtc_logging.h"

void
test_logging_level()
{
    // all uppercase
    assert(jrtc_get_logging_level("DEBUG") == JRTC_DEBUG_LEVEL);
    assert(jrtc_get_logging_level("INFO") == JRTC_INFO_LEVEL);
    assert(jrtc_get_logging_level("WARN") == JRTC_WARN_LEVEL);
    assert(jrtc_get_logging_level("WARNING") == JRTC_WARN_LEVEL);
    assert(jrtc_get_logging_level("ERROR") == JRTC_ERROR_LEVEL);
    assert(jrtc_get_logging_level("CRITICAL") == JRTC_CRITICAL_LEVEL);
    assert(jrtc_get_logging_level("FATAL") == JRTC_CRITICAL_LEVEL);
    assert(jrtc_get_logging_level("UNKNOWN") == JRTC_DEBUG_LEVEL); // Default to DEBUG for unknown

    // all lowercase
    assert(jrtc_get_logging_level("debug") == JRTC_DEBUG_LEVEL);
    assert(jrtc_get_logging_level("info") == JRTC_INFO_LEVEL);
    assert(jrtc_get_logging_level("warn") == JRTC_WARN_LEVEL);
    assert(jrtc_get_logging_level("warning") == JRTC_WARN_LEVEL);
    assert(jrtc_get_logging_level("error") == JRTC_CRITICAL_LEVEL);
    assert(jrtc_get_logging_level("critical") == JRTC_CRITICAL_LEVEL);
    assert(jrtc_get_logging_level("fatal") == JRTC_CRITICAL_LEVEL);
}

void
test_set_and_get_logging_level()
{
    // Set logging level to INFO
    jrtc_set_logging_level(JRTC_INFO_LEVEL);
    assert(jrtc_get_current_logging_level() == JRTC_INFO_LEVEL);

    // Set logging level to DEBUG
    jrtc_set_logging_level(JRTC_DEBUG_LEVEL);
    assert(jrtc_get_current_logging_level() == JRTC_DEBUG_LEVEL);

    // Set logging level to ERROR
    jrtc_set_logging_level(JRTC_ERROR_LEVEL);
    assert(jrtc_get_current_logging_level() == JRTC_ERROR_LEVEL);

    // Set logging level to CRITICAL
    jrtc_set_logging_level(JRTC_CRITICAL_LEVEL);
    assert(jrtc_get_current_logging_level() == JRTC_CRITICAL_LEVEL);

    // Set logging level to WARN
    jrtc_set_logging_level(JRTC_WARN_LEVEL);
    assert(jrtc_get_current_logging_level() == JRTC_WARN_LEVEL);
}

int
main()
{
    printf("Running logging level tests...\n");
    test_logging_level();
    printf("Logging level tests passed.\n");
    test_set_and_get_logging_level();
    printf("Set and get logging level tests passed.\n");
    return 0;
}
