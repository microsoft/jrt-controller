// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jrtc_logging.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static const char* STR[] = {FOREACH_LOG_LEVEL(GENERATE_STRING)};

void
jrtc_default_va_logging(const char* domain, jrtc_logging_level level, const char* s, va_list arg)
{
    char output[LOGGING_BUFFER_LEN];
    snprintf(output, LOGGING_BUFFER_LEN, "%s %s%s", domain, STR[level], s);

    if (level >= jrtc_logger_level) {
        FILE* where = level >= INFO ? stderr : stdout;
        vfprintf(where, output, arg);
        fflush(where);
    }
}

void
jrtc_default_logging(const char* domain, jrtc_logging_level level, const char* s, ...)
{
    if (level >= jrtc_logger_level) {
        char output[LOGGING_BUFFER_LEN];
        snprintf(output, LOGGING_BUFFER_LEN, "%s %s%s", domain, STR[level], s);
        va_list ap;
        va_start(ap, s);
        FILE* where = level >= INFO ? stderr : stdout;
        vfprintf(where, output, ap);
        va_end(ap);
        fflush(where);
    }
}

void
jrtc_set_logging_level(jrtc_logging_level level)
{
    jrtc_logger_level = level;
}

void
jrtc_set_logging_function(jrtc_logger_function_type func)
{
    jrtc_logger = func;
}

void
jrtc_set_va_logging_function(jrtc_va_logger_function_type func)
{
    jrtc_va_logger = func;
}

jrtc_logging_level jrtc_logger_level = DEBUG;
jrtc_logger_function_type jrtc_logger = jrtc_default_logging;
jrtc_va_logger_function_type jrtc_va_logger = jrtc_default_va_logging;
