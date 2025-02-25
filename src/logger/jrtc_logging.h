// Copyright (c) Microsoft Corporation. All rights reserved.

#ifndef JRTC_LOGGING_H
#define JRTC_LOGGING_H

#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>

#pragma once
#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief The logging buffer length
 * @ingroup logger
 */
#define LOGGING_BUFFER_LEN 8192

/**
 * @brief Wrap the string in a log format
 * @param s The string
 */
#define JRTC_LOG_WRAP(s) \
    "["s                 \
    "]: "

// jrtc domain
#define FOREACH_LOG_LEVEL(LOG_LEVEL) \
    LOG_LEVEL(DEBUG)                 \
    LOG_LEVEL(INFO)                  \
    LOG_LEVEL(WARN)                  \
    LOG_LEVEL(ERROR)                 \
    LOG_LEVEL(CRITICAL)

#define _STR(x) _VAL(x)
#define _VAL(x) #x

    // Function to generate the log prefix dynamically
    static inline const char*
    get_log_prefix(const char* file, const char* func, int line, const char* level)
    {
        static char buffer[256];
        snprintf(buffer, sizeof(buffer), "[%s:%s:%s:%d]", level, file, func, line);
        return buffer;
    }

// Define macros using the helper function
#define JRTC_CRITICAL get_log_prefix(__FILE__, __func__, __LINE__, "CRITICAL"), CRITICAL
#define JRTC_ERROR get_log_prefix(__FILE__, __func__, __LINE__, "ERROR"), ERROR
#define JRTC_WARN get_log_prefix(__FILE__, __func__, __LINE__, "WARN"), WARN
#define JRTC_INFO get_log_prefix(__FILE__, __func__, __LINE__, "INFO"), INFO
#define JRTC_DEBUG get_log_prefix(__FILE__, __func__, __LINE__, "DEBUG"), DEBUG

#define GENERATE_ENUM_LOG(ENUM) ENUM,
#define GENERATE_STRING(STRING) JRTC_LOG_WRAP(#STRING),

    typedef enum
    {
        FOREACH_LOG_LEVEL(GENERATE_ENUM_LOG)
    } jrtc_logging_level;

    /**
     * @brief The logger function type
     * @param domain The domain
     */
    typedef void (*jrtc_logger_function_type)(const char*, jrtc_logging_level, const char*, ...);

    /**
     * @brief The variant argument logger function type
     * @param domain The domain
     */
    typedef void (*jrtc_va_logger_function_type)(const char*, jrtc_logging_level, const char*, va_list arg);

    /**
     * @brief The jrtc_logger_level
     * @ingroup logger
     */
    extern jrtc_logging_level jrtc_logger_level;

    /**
     * @brief The jrtc_logger
     * @ingroup logger
     */
    extern jrtc_logger_function_type jrtc_logger;

    /**
     * @brief The jrtc_va_logger
     * @ingroup logger
     */
    extern jrtc_va_logger_function_type jrtc_va_logger;

    /**
     * @brief The jrtc_default_logging
     * @ingroup logger
     * @param domain The domain
     * @param level The logging level
     * @param s The string
     */
    void
    jrtc_default_logging(const char* domain, jrtc_logging_level level, const char* s, ...);

    /**
     * @brief The jrtc_default_va_logging
     * @ingroup logger
     * @param domain The domain
     * @param level The logging level
     * @param s The string
     * @param arg The variant argument
     */
    void
    jrtc_default_va_logging(const char* domain, jrtc_logging_level level, const char* s, va_list arg);

    /**
     * @brief Set the logging level
     * @ingroup logger
     * @param level The logging level
     */
    void
    jrtc_set_logging_level(jrtc_logging_level level);

    /**
     * @brief Set the logging function
     * @ingroup logger
     * @param func The logging function
     */
    void
    jrtc_set_logging_function(jrtc_logger_function_type func);

    /**
     * @brief Set the variant argument logging function
     * @ingroup logger
     * @param func The variant argument logging function
     */
    void
    jrtc_set_va_logging_function(jrtc_va_logger_function_type func);

#pragma once
#ifdef __cplusplus
}
#endif

#endif /* JRTC_LOGGING_H */
