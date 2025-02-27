// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
/**
    This test tests the controller API by initializing the controller and stopping it.
    It creates two threads, one to stop the jrt-controller after 5 seconds and another to start the jrtc.
*/
#include "jrtc_int.h"
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include "jrtc_logging.h"

// Thread Function to stop the jrtc
void*
stop_jrtc_func(void* args)
{
    jrtc_logger(JRTC_INFO, "Waiting for 5 seconds before stopping jrtc.\n");
    sleep(5);
    jrtc_logger(JRTC_INFO, "Stopping jrtc.\n");
    stop_jrtc();
    jrtc_logger(JRTC_INFO, "Stop signal sent.\n");
    return NULL;
}

// Thread Function to start the jrtc
void*
start_jrtc_func(void* args)
{
    jrtc_logger(JRTC_INFO, "Starting jrtc now.\n");
    int res = start_jrtc(0, NULL);
    if (res < 0) {
        jrtc_logger(JRTC_CRITICAL, "jrt-controller failed to start\n");
    }
    assert(res >= 0);
    jrtc_logger(JRTC_INFO, "jrt-controller ended.\n");
    return NULL;
}

int
main(int argc, char* argv[])
{
    // create a thread to stop the jrtc after 5 seconds
    pthread_t stop_thread;
    int res = pthread_create(&stop_thread, NULL, stop_jrtc_func, NULL);
    assert(res == 0);

    // create a thread to start the jrtc
    pthread_t jrtc_thread;
    res = pthread_create(&jrtc_thread, NULL, start_jrtc_func, NULL);
    assert(res == 0);

    // join the stop thread
    res = pthread_join(stop_thread, NULL);
    assert(res == 0);

    // join the jrtc thread
    res = pthread_join(jrtc_thread, NULL);
    assert(res == 0);

    // cleanup
    jrtc_logger(JRTC_INFO, "Test completed successfully.\n");
    return 0;
}