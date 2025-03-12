// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
/**
    This test tests the jrtc by forking a child process to act as an agent and the parent process to act as a router.
    In the process of router, two applications are registered and data is sent and received between the agent and the
   applications. The test is successful if the data sent by the agent is received by the applications.
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <sys/mman.h>
#include <assert.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/wait.h>

#include "jrtc_router.h"
#include "jrtc_router_app_api.h"
#include "jrtc_router_stream_id.h"

#include "jrtc_logging.h"

#include "jbpf_io.h"
#include "jbpf_io_channel.h"
#include "jrtc_logging.h"
#include "jrtc_yaml_int.h"

struct test_struct
{
    uint32_t counter_a;
};

bool* done;

#define AGENT_SEM_NAME "/AGENT_SEM"
#define ROUTER_SEM_NAME "/ROUTER_SEM"

pid_t cpid = -1;

sem_t *agent_sem, *router_sem;

// Signal handler
void
ctrlc_handler(int signo)
{
    if (signo == SIGINT) {
        jrtc_logger(JRTC_INFO, "Ctrl+C pressed. Cleaning up...\n");
        *done = true;
    }
}

void*
test_app(void* args)
{

    dapp_router_ctx_t dapp_ctx;
    jrtc_router_stream_id_t stream_id_req;
    int num_rcv, res;
    struct test_struct* data;

    jrtc_router_data_entry_t data_entries[100] = {0};

    jrtc_router_generate_stream_id(
        &stream_id_req,
        JRTC_ROUTER_REQ_DEST_ANY,
        JRTC_ROUTER_REQ_DEVICE_ID_ANY,
        JRTC_ROUTER_REQ_STREAM_PATH_ANY,
        JRTC_ROUTER_REQ_STREAM_NAME_ANY);

    dapp_ctx = jrtc_router_register_app(100);

    if (!dapp_ctx) {
        jrtc_logger(JRTC_ERROR, "Application could not be registered successfully\n");
        return NULL;
    } else {
        jrtc_logger(JRTC_INFO, "Application 1 registered successfully\n");
    }

    res = jrtc_router_channel_register_stream_id_req(dapp_ctx, stream_id_req);

    if (res == 1) {
        jrtc_logger(JRTC_INFO, "Stream id 1 registered successfully\n");
    }

    while (!*done) {
        num_rcv = jrtc_router_receive(dapp_ctx, data_entries, 100);
        if (num_rcv > 0) {
            for (int i = 0; i < num_rcv; i++) {
                data = data_entries[i].data;
                jrtc_logger(JRTC_INFO, "App 1: Received message %d\n", data->counter_a);
                jrtc_router_channel_release_buf(data);
            }
        }
        usleep(1000);
    }

    jrtc_logger(JRTC_INFO, "App 1 exiting\n");

    jrtc_router_deregister_app(dapp_ctx);

    return NULL;
}

void*
test_app2(void* args)
{

    dapp_router_ctx_t dapp_ctx;
    jrtc_router_stream_id_t stream_id_req;
    int num_rcv, res;
    struct test_struct* data;
    int received_counter = 0;

    jrtc_router_data_entry_t data_entries[100] = {0};

    dapp_ctx = jrtc_router_register_app(100);

    if (!dapp_ctx) {
        jrtc_logger(JRTC_ERROR, "Application 2 could not be registered successfully\n");
        return NULL;
    }
    jrtc_logger(JRTC_INFO, "Application 2 registered successfully\n");

    jrtc_router_generate_stream_id(&stream_id_req, JRTC_ROUTER_DEST_UDP, 0, "codelet1", "map1");

    res = jrtc_router_channel_register_stream_id_req(dapp_ctx, stream_id_req);

    if (res == 1) {
        jrtc_logger(JRTC_INFO, "Stream id for app 2 registered successfully\n");
    }

    while (!*done) {
        num_rcv = jrtc_router_receive(dapp_ctx, data_entries, 100);
        if (num_rcv > 0) {
            received_counter += num_rcv;
            for (int i = 0; i < num_rcv; i++) {
                data = data_entries[i].data;
                jrtc_logger(JRTC_INFO, "App 2: Received message %d\n", data->counter_a);
                jrtc_router_channel_release_buf(data);
            }
        }
        if (received_counter >= 10) {
            jrtc_router_channel_deregister_stream_id_req(dapp_ctx, stream_id_req);
            received_counter = 0;
        }
        usleep(1000);
    }

    jrtc_logger(JRTC_INFO, "App 2 exiting\n");

    jrtc_router_channel_deregister_stream_id_req(dapp_ctx, stream_id_req);

    jrtc_router_deregister_app(dapp_ctx);

    return NULL;
}

int
router_test()
{

    struct yaml_config config = {0};
    pthread_t test_app_tid, test_app2_tid;
    int res;

    config.jrtc_router_config.thread_config.affinity_mask = 1 << 1;
    config.jrtc_router_config.thread_config.has_affinity_mask = false;
    config.jrtc_router_config.thread_config.has_sched_config = false;
    config.jrtc_router_config.thread_config.sched_config.sched_policy = JRTC_ROUTER_DEADLINE;
    config.jrtc_router_config.thread_config.sched_config.sched_priority = 99;
    config.jrtc_router_config.thread_config.sched_config.sched_deadline = 30 * 1000 * 1000;
    config.jrtc_router_config.thread_config.sched_config.sched_runtime = 10 * 1000 * 1000;
    config.jrtc_router_config.thread_config.sched_config.sched_period = 30 * 1000 * 1000;

    strncpy(config.jrtc_router_config.io_config.ipc_name, "jrtc_router_test", 32);

    res = jrtc_router_init(&config);

    if (res < 0) {
        jrtc_logger(JRTC_ERROR, "Failed to initialize router\n");
    } else {
        jrtc_logger(JRTC_INFO, "Router initialized successfully\n");
    }

    // Create some test application thread
    pthread_create(&test_app_tid, NULL, test_app, NULL);
    pthread_create(&test_app2_tid, NULL, test_app2, NULL);

    sem_post(router_sem);

    pthread_join(test_app_tid, NULL);
    pthread_join(test_app2_tid, NULL);

    jrtc_router_stop();
    return 0;
}

int
agent_test()
{
    struct jbpf_io_ctx* io_ctx;
    struct jbpf_io_config io_config = {0};
    jbpf_io_channel_t* io_channel;
    struct jbpf_io_stream_id stream_id;

    char descriptor[65535] = {0};
    io_config.type = JBPF_IO_IPC_PRIMARY;
    io_config.ipc_config.mem_cfg.memory_size = 1024 * 1024 * 1024;
    strncpy(io_config.ipc_config.addr.jbpf_io_ipc_name, "jrtc_router_test", JBPF_IO_IPC_MAX_NAMELEN - 1);

    // Wait until the primary is ready
    sem_wait(router_sem);
    io_ctx = jbpf_io_init(&io_config);
    if (io_ctx == NULL) {
        jrtc_logger(JRTC_ERROR, "Could not create IO context\n");
        exit(1);
    } else {
        jrtc_logger(JRTC_INFO, "Socket created successfully\n");
    }

    jbpf_io_register_thread();

    jrtc_router_generate_stream_id((jrtc_router_stream_id_t*)&stream_id, JRTC_ROUTER_DEST_UDP, 0, "codelet1", "map1");

    // Create an output channel
    io_channel = jbpf_io_create_channel(
        io_ctx,
        JBPF_IO_CHANNEL_OUTPUT,
        JBPF_IO_CHANNEL_QUEUE,
        10,
        sizeof(struct test_struct),
        stream_id,
        descriptor,
        0);

    if (!io_channel) {
        jrtc_logger(JRTC_ERROR, "Channel could not be created successfully\n");
    }

    int counter = 0;

    for (int i = 0; i < 3; i++) {
        jrtc_logger(JRTC_INFO, "Sending data %d\n", counter);
        // Reserve a buffer, populate it and send some data
        struct test_struct* data = jbpf_io_channel_reserve_buf(io_channel);

        if (!data) {
            jrtc_logger(JRTC_ERROR, "Could not reserve buffer\n");
            sleep(1);
            continue;
        }
        data->counter_a = counter++;

        jbpf_io_channel_submit_buf(io_channel);
        sleep(1);
    }

    jrtc_logger(JRTC_INFO, "Agent completed successfully\n");
    *done = true;
    return 0;
}

void
handle_sigchld(int sig)
{
    return;
}

void
handle_sigterm(int sig)
{
    if (cpid > 0) {
        kill(cpid, SIGKILL);
    }
    exit(EXIT_FAILURE);
}

int
main(int argc, char* argv[])
{
    // Create shared memory for the `done` flag
    done = mmap(NULL, sizeof(*done), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (done == MAP_FAILED) {
        perror("mmap failed");
        return 1;
    }
    *done = false;

    pid_t child_pid;
    // Register some signals to kill the test, if it fails
    struct sigaction sa;
    sa.sa_handler = &handle_sigchld;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa, 0) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
    struct sigaction sa_child;
    sa_child.sa_handler = &handle_sigterm;
    sigemptyset(&sa_child.sa_mask);
    sa_child.sa_flags = 0;
    if (sigaction(SIGTERM, &sa_child, 0) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
    if (sigaction(SIGINT, &sa_child, 0) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
    if (sigaction(SIGABRT, &sa_child, 0) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    // Remove the semaphore if the test did not finish gracefully
    sem_unlink(ROUTER_SEM_NAME);
    sem_unlink(AGENT_SEM_NAME);

    router_sem = sem_open(ROUTER_SEM_NAME, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0);
    if (router_sem == SEM_FAILED) {
        exit(EXIT_FAILURE);
    }

    agent_sem = sem_open(AGENT_SEM_NAME, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0);
    if (agent_sem == SEM_FAILED) {
        exit(EXIT_FAILURE);
    }

    child_pid = fork();
    assert(child_pid >= 0);

    if (child_pid == 0) {
        // secondary
        int res = agent_test();
        assert(res == 0);
    } else {
        // primary
        cpid = child_pid;
        router_test();
        printf("Test completed successfully\n");
    }

    // cleanup
    munmap((void*)done, sizeof(*done));
    sem_unlink(AGENT_SEM_NAME);
    sem_unlink(ROUTER_SEM_NAME);
}
