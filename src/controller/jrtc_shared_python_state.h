#ifndef JRTC_GLOBALS_H
#define JRTC_GLOBALS_H

#include <stdatomic.h>
#include <pthread.h>

typedef struct shared_python_state
{
    pthread_mutex_t python_lock;
    PyThreadState* main_ts; // Store the main interpreter thread state
    bool initialized;
} shared_python_state_t;

#endif
