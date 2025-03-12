// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#define _GNU_SOURCE
#include "jrtc_int.h"
#include "jrtc_logging.h"
#include <sys/mman.h>
#include <Python.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <dlfcn.h>
#include <string.h>
#include <semaphore.h>
#include <stdatomic.h>

#include "jrtc.h"
#include "jrtc_router.h"

#include "jrtc_router_stream_id.h"

#include "jbpf_io.h"
#include "jbpf_io_defs.h"
#include "jbpf_io_queue.h"
#include "jbpf_io_channel.h"
#include "jrtc_int.h"

#include "jrtc_rest_server.h"
#include "jrtc_logging.h"
#include "jrtc_yaml_int.h"
#include "jrtc_yaml.h"

/* Compiler magic to make address sanitizer ignore
memory leaks originating from libpython */
#if defined(__SANITIZE_ADDRESS__) || defined(__SANITIZE_LEAK__)
__attribute__((used)) const char*
__asan_default_options()
{
    return "detect_odr_violation=0:intercept_tls_get_addr=0:suppression=libpython";
}

__attribute__((used)) const char*
__lsan_default_options()
{
    return "print_suppressions=0";
}

__attribute__((used)) const char*
__lsan_default_suppressions()
{
    return "leak:libpython";
}
#endif

char*
concat(const char* s1, const char* s2)
{
    // Calculate the length of the concatenated string
    size_t len1 = strlen(s1);
    size_t len2 = strlen(s2);
    size_t total_len = len1 + len2 + 1; // +1 for the null terminator

    // Allocate memory for the concatenated string
    char* result = (char*)malloc(total_len * sizeof(char));
    if (result == NULL) {
        jrtc_logger(JRTC_CRITICAL, "Memory allocation failed\n");
        exit(1);
    }

    // Copy the first string
    strcpy(result, s1);

    // Concatenate the second string
    strcat(result, s2);

    return result;
}

#define MAX_NUM_APPS 20

#define NORTH_IO_LIB "libjrtc_north_io.so"
#define MAX_NUM_JRTC_APPS 64

sem_t jrtc_stop;

struct jrtc_app_env* app_envs[MAX_NUM_JRTC_APPS] = {NULL};
int next_available_app_env = 0;

static char*
read_file(const char* filename, size_t* size)
{
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        jrtc_logger(JRTC_CRITICAL, "Failed to open file %s\n", filename);
        return NULL;
    }

    // Seek to the end of the file to determine its size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    if (file_size == -1) {
        jrtc_logger(JRTC_CRITICAL, "Failed to determine file size\n");
        fclose(file);
        return NULL;
    }

    // Allocate memory for the char array
    char* buffer = (char*)calloc(1, file_size + 1); // Add 1 for null terminator
    if (buffer == NULL) {
        jrtc_logger(JRTC_CRITICAL, "Failed to allocate memory\n");
        fclose(file);
        return NULL;
    }

    // Rewind to the beginning of the file
    rewind(file);

    // Read the contents of the file into the char array
    if (fread(buffer, 1, file_size, file) != file_size) {
        jrtc_logger(JRTC_CRITICAL, "Failed to read file\n");
        free(buffer);
        fclose(file);
        return NULL;
    }
    buffer[file_size] = '\0'; // Null-terminate the string

    // Set the size of the file
    if (size != NULL) {
        *size = (size_t)file_size;
    }

    // Close the file
    fclose(file);

    return buffer;
}

// Function to load the library from memory
static void*
_jrtc_load_app_from_memory(const char* data, size_t size)
{
    char path[100];

    // Create a memory-mapped file descriptor
    int mem_fd = memfd_create("jrtc_app", 0);
    if (mem_fd == -1) {
        jrtc_logger(JRTC_CRITICAL, "memfd_create failed\n");
        return NULL;
    }

    if (ftruncate(mem_fd, size) == -1)
        goto error;

    if (write(mem_fd, data, size) != size) {
        jrtc_logger(JRTC_CRITICAL, "write to memfd failed\n");
        goto error;
    }

    snprintf(path, sizeof(path), "/proc/self/fd/%d", mem_fd);

    void* handle = dlopen(path, RTLD_LAZY);
    if (handle == NULL) {
        jrtc_logger(JRTC_CRITICAL, "fdlopen failed: %s (errno=%d, %s)\n", path, errno, dlerror());
        goto error;
    }

    close(mem_fd);
    return handle;

error:
    close(mem_fd);
    return NULL;
}

void*
run_app(void* args)
{

    struct jrtc_app_env* app_env;

    app_env = args;

    app_env->dapp_ctx = jrtc_router_register_app(app_env->io_queue_size);

    if (app_env->sched_config.sched_deadline_us > 0) {
        jrtc_thread_set_scheduler(app_env->app_tid, &app_env->sched_config);
    }

    if (pthread_setname_np(app_env->app_tid, app_env->app_name)) {
        jrtc_logger(JRTC_CRITICAL, "Error in setting app name to %s\n", app_env->app_name);
    } else {
        jrtc_logger(JRTC_INFO, "Set name %s successfully\n", app_env->app_name);
    }

    void* (*app)(void* args) = dlsym(app_env->app_handle, "jrtc_start_app");
    if (app) {
        app(app_env);
    } else {
        dlclose(app_env->app_handle);
    }

    return NULL;
}

static int
_jrtc_reserve_app_id(struct jrtc_app_env* app_env)
{
    for (int i = 0; i < MAX_NUM_JRTC_APPS; i++) {
        int index = (next_available_app_env + i) % MAX_NUM_JRTC_APPS;
        if (app_envs[index] == NULL) {
            app_envs[index] = app_env;
            next_available_app_env = (index + 1) % MAX_NUM_JRTC_APPS;
            return index;
        }
    }
    return -1; // No available slots
}

static void
_jrtc_release_app_id(int app_id)
{
    if (app_id >= 0 && app_id < MAX_NUM_JRTC_APPS) {
        if (app_envs[app_id] == NULL) {
            return;
        }
        free(app_envs[app_id]);
        app_envs[app_id] = NULL;
    }
}

int
load_app(load_app_request_t load_req)
{

    struct jrtc_app_env* app_env;

    if (!load_req.app || !load_req.app_size)
        return -1;

    app_env = calloc(1, sizeof(struct jrtc_app_env));

    if (!app_env)
        return -1;

    int app_id = _jrtc_reserve_app_id(app_env);

    if (app_id < 0) {
        jrtc_logger(JRTC_CRITICAL, "Could not reserve resources for new app %s\n", load_req.app_name);
        return -1;
    }

    void* app_handle = _jrtc_load_app_from_memory(load_req.app, load_req.app_size);
    if (!app_handle)
        goto error;

    app_env->app_handle = app_handle;
    app_env->app_exit = false;
    app_env->io_queue_size = load_req.ioq_size;
    memset(app_env->app_params, 0, sizeof(app_env->app_params));

    for (int i = 0; i < MAX_APP_PARAMS; i++) {
        if (load_req.app_params[i] != NULL) {
            app_env->app_params[i] = strdup(load_req.app_params[i]);
        }
    }

    app_env->sched_config.sched_runtime_us = load_req.runtime_us;
    app_env->sched_config.sched_period_us = load_req.period_us;
    app_env->sched_config.sched_deadline_us = load_req.deadline_us;
    if (app_env->sched_config.sched_deadline_us > 0)
        app_env->sched_config.sched_policy = JRTC_SCHED_DEADLINE;

    if (load_req.app_name == NULL) {
        app_env->app_name = strdup("jrtc_app");
    } else {
        app_env->app_name = strdup(load_req.app_name);
    }

    int res = pthread_create(&app_env->app_tid, NULL, run_app, app_env);
    if (res != 0) {
        jrtc_logger(JRTC_CRITICAL, "Failed to create thread for app %s\n", app_env->app_name);
        goto load_app_error;
    }
    return app_id;

load_app_error:
    dlclose(app_handle);
error:
    _jrtc_release_app_id(app_id);
    free(app_env);
    return -1;
}

int
unload_app(int app_id)
{
    struct timespec timeout;
    if (app_envs[app_id] == NULL) {
        return -1;
    }

    jrtc_logger(JRTC_INFO, "Shutting down app %s\n", app_envs[app_id]->app_name);
    atomic_store(&app_envs[app_id]->app_exit, true);
    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_sec += 2;
    pthread_timedjoin_np(app_envs[app_id]->app_tid, NULL, &timeout);
    dlclose(app_envs[app_id]->app_handle);
    jrtc_logger(JRTC_INFO, "Deregistering app %s\n", app_envs[app_id]->app_name);
    jrtc_router_deregister_app(app_envs[app_id]->dapp_ctx);
    jrtc_logger(JRTC_INFO, "App %s shut down\n", app_envs[app_id]->app_name);
    free(app_envs[app_id]->app_name);
    _jrtc_release_app_id(app_id);
    return 0;
}

int
load_default_north_io_app()
{
    size_t north_io_size = 0;
    char* north_io_data;
    char* north_io_app_name;

    // Create default IO app
    char* app_path = getenv("JRTC_APP_PATH");
    if (app_path != NULL) {
        jrtc_logger(JRTC_INFO, "JRTC_APP_PATH environment variable: %s\n", app_path);
        north_io_app_name = concat(app_path, NORTH_IO_LIB);
    } else {
        jrtc_logger(JRTC_INFO, "JRTC_APP_PATH environment variable is not set. Using default path\n");
        north_io_app_name = strdup(NORTH_IO_LIB);
    }

    jrtc_logger(JRTC_INFO, "Loading north io app from %s\n", north_io_app_name);
    north_io_data = read_file(north_io_app_name, &north_io_size);

    if (!north_io_data) {
        jrtc_logger(JRTC_CRITICAL, "Could not find %s app\n", north_io_app_name);
        free(north_io_app_name);
        return -1;
    }

    load_app_request_t load_req_north_io;
    load_req_north_io.app = north_io_data;
    load_req_north_io.app_size = north_io_size;
    load_req_north_io.ioq_size = 1000;

    load_req_north_io.deadline_us = 0;
    load_req_north_io.app_name = strdup("north_io_app");
    memset(load_req_north_io.app_params, 0, sizeof(load_req_north_io.app_params));

    int res = load_app(load_req_north_io);
    if (res == 0) {
        jrtc_logger(JRTC_INFO, "Loaded north io app\n");
    } else {
        jrtc_logger(JRTC_CRITICAL, "Failed to load north io app\n");
    }

    free(load_req_north_io.app_name);
    free(north_io_app_name);
    free(north_io_data);
    return res;
}

// Signal handler
void
ctrlc_handler(int signo)
{
    if (signo == SIGINT) {
        printf("Ctrl+C pressed. Cleaning up...\n");
        sem_post(&jrtc_stop);
        if (Py_IsInitialized()) {
            Py_Finalize();
        }
    }
}

void*
_start_rest_server(void* args)
{

    unsigned int port = 3001;

    if (pthread_setname_np(pthread_self(), "jrtc_rest")) {
        jrtc_logger(JRTC_CRITICAL, "Error in setting app name to %s\n", "jrtc_rest");
    } else {
        jrtc_logger(JRTC_INFO, "Set name %s successfully\n", "jrtc_rest");
    }

    jrtc_rest_callbacks callbacks;
    callbacks.load_app = load_app;
    callbacks.unload_app = unload_app;

    jrtc_start_rest_server(args, port, &callbacks);

    return NULL;
}

int
start_jrtc(const char* config_file)
{
    if (signal(SIGINT, ctrlc_handler) == SIG_ERR) {
        perror("signal");
        return 1;
    }

    pthread_t rest_server;
    void* rest_server_handle;

    struct jrtc_router_config config = {0};
    int res;

    sem_init(&jrtc_stop, 0, 0);

    rest_server_handle = jrtc_create_rest_server();
    pthread_create(&rest_server, NULL, _start_rest_server, rest_server_handle);

    yaml_config_t yaml_config;
    res = set_config_values(config_file, &yaml_config);
    if (res != 0) {
        jrtc_logger(JRTC_ERROR, "Failed to read thread config from YAML file: %s (%d)\n", config_file, res);
        return -2;
    }
    strncpy(config.io_config.ipc_name, "jrtc_controller", 32);

    res = jrtc_router_init(&yaml_config);

    if (res < 0) {
        jrtc_logger(JRTC_CRITICAL, "Failed to initialize router\n");
    } else {
        jrtc_logger(JRTC_INFO, "Router initialized successfully\n");
    }

    if (pthread_setname_np(pthread_self(), "jrtc_main")) {
        jrtc_logger(JRTC_CRITICAL, "Error in setting app name to %s\n", "jrtc_main");
    } else {
        jrtc_logger(JRTC_INFO, "Set name %s successfully\n", "jrtc_main");
    }

    res = load_default_north_io_app();
    if (res < 0) {
        jrtc_logger(JRTC_CRITICAL, "Failed to load default north io app\n");
    } else {
        jrtc_logger(JRTC_INFO, "Default north io app started\n");
    }

    sem_wait(&jrtc_stop);

    // Stop REST server
    jrtc_logger(JRTC_INFO, "Stopping REST server\n");
    jrtc_stop_rest_server(rest_server_handle);

    for (int i = 0; i < MAX_NUM_JRTC_APPS; i++) {
        if (app_envs[i] == NULL) {
            continue;
        }
        if (unload_app(i) == 0) {
            jrtc_logger(JRTC_INFO, "App %d unloaded\n", i);
        } else {
            jrtc_logger(JRTC_ERROR, "Failed to unload app %d\n", i);
            res = -1;
        }
    }

    jrtc_logger(JRTC_INFO, "Stopping router\n");
    if (jrtc_router_stop() < 0) {
        jrtc_logger(JRTC_ERROR, "Failed to stop router\n");
        res = -1;
    }

    jrtc_logger(JRTC_INFO, "jrt-controller stopped.\n");

    sem_destroy(&jrtc_stop);
    return res;
}

void
stop_jrtc()
{
    sem_post(&jrtc_stop);
}
