#ifndef JRTC_YAML_INT_H
#define JRTC_YAML_INT_H

// Forward declare to avoid circular dependency
struct jrtc_router_config;

#include "jbpf_io_defs.h"

struct yaml_config
{
    struct jrtc_router_config jrtc_router_config;
    struct jbpf_io_config jbpf_io_config;
};

typedef struct yaml_config yaml_config_t;

#endif
