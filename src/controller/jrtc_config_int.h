#ifndef JRTC_YAML_INT_H
#define JRTC_YAML_INT_H

// Forward declare to avoid circular dependency
struct jrtc_router_config;

#include "jbpf_io_defs.h"

struct jrtc_config
{
    struct jrtc_router_config jrtc_router_config;
    struct jbpf_io_config jbpf_io_config;
};

typedef struct jrtc_config jrtc_config_t;

#endif
