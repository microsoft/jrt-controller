// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include <unistd.h>
#include <signal.h>

#include "jbpf.h"
#include "jbpf_hook.h"
#include "jbpf_defs.h"

#define MAX_ITER 2000

struct packet
{
    int counter_a;
    int counter_b;
};

// Hook declaration and definition.
DECLARE_JBPF_HOOK(
    test1,
    struct jbpf_generic_ctx ctx,
    ctx,
    HOOK_PROTO(struct packet* p, int ctx_id),
    HOOK_ASSIGN(ctx.ctx_id = ctx_id; ctx.data = (uint64_t)(void*)p; ctx.data_end = (uint64_t)(void*)(p + 1);))

DEFINE_JBPF_HOOK(test1)

double g_ticks_per_ns;
uint64_t g_tick_freq;

static int sig_handler_called = 0;

void
sig_handler(int signo)
{
    if (sig_handler_called == 1) {
        return;
    }
    sig_handler_called = 1;
    printf("Exiting Jbpf...\n");
    jbpf_cleanup_thread();
    jbpf_stop();
    printf("OK! Goodbye!\n");
    exit(EXIT_SUCCESS);
}

int
handle_signal()
{
    if (signal(SIGINT, sig_handler) == SIG_ERR) {
        return 0;
    }
    if (signal(SIGTERM, sig_handler) == SIG_ERR) {
        return 0;
    }
    return -1;
}

int
main()
{
    int ret;

    if (!handle_signal()) {
        printf("Could not handle SIGINT signal\n");
        exit(1);
    }

    struct jbpf_config jbpf_config = {0};
    jbpf_set_default_config_options(&jbpf_config);

    jbpf_config.io_config.io_type = JBPF_IO_IPC_CONFIG;
    jbpf_config.io_config.io_ipc_config.ipc_mem_size = JBPF_HUGEPAGE_SIZE_1GB;
    strncpy(jbpf_config.io_config.io_ipc_config.ipc_name, "jrt_controller", JBPF_IO_IPC_MAX_NAMELEN);

    // Enable LCM IPC interface using UNIX socket at the default socket path (the default is through C API)
    jbpf_config.lcm_ipc_config.has_lcm_ipc_thread = true;
    snprintf(
        jbpf_config.lcm_ipc_config.lcm_ipc_name,
        sizeof(jbpf_config.lcm_ipc_config.lcm_ipc_name) - 1,
        "%s",
        JBPF_DEFAULT_LCM_SOCKET);

    if (jbpf_init(&jbpf_config) < 0) {
        printf("Could not initialize Jbpf.\n");
        exit(1);
    }

    jbpf_register_thread();

    struct packet p = {1, 2};

    // uint64_t start_time, end_time, elapsed_time;
    while (1) {
        // start_time = jbpf_start_time();
        hook_test1(&p, 1);
        usleep(1000000);
    }

    jbpf_stop();
    exit(EXIT_SUCCESS);

    return ret;
}
