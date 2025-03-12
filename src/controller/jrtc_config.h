// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#ifndef JRTC_YAML_H
#define JRTC_YAML_H

#include "jrtc_config_int.h"

/**
 * @brief Expands environment variables in a given string (e.g., "Path: ${HOME}/test")
 * @param input - The input string with possible ${VAR} placeholders.
 * @return - The expanded string (must be freed by the caller).
 */
char*
expand_env_vars(const char* input);

/**
 * @brief Set configuration values.
 * @param filename The path to the YAML file (Optional, could be set to NULL).
 * @param config Pointer to the structure to fill with parsed data.
 * @return 0 on success, -1 on failure.
 * @note the default values are set in the jrtc_config_t structure and when
 *       the filename is not NULL, the values are overridden with the ones in the file.
 */
int
set_config_values(const char* filename, jrtc_config_t* config);

/**
 * @brief Initialize the YAML configuration with default values.
 * @param config Pointer to the structure to fill with default values.
 * @note This function sets the default values for the YAML configuration.
 *       It should be called before using the configuration structure.
 */
void
init_jrtc_config(jrtc_config_t* config);

#endif // JRTC_YAML_H