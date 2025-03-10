// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#ifndef JRTC_INT_H
#define JRTC_INT_H

/**
 * @brief Concatenate two strings
 * @param s1 The first string
 * @param s2 The second string
 * @return The concatenated string
 */
char*
concat(const char* s1, const char* s2);

/**
 * @brief Start the JRTC controller
 * @param config_file The configuration file (YAML format)
 * @return 0 on success, -1 on failure
 */
int
start_jrtc(const char* config_file);

/**
 * @brief Stop the JRTC controller
 */
void
stop_jrtc();

#endif
