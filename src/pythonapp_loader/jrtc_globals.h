#ifndef JRTC_GLOBALS_H
#define JRTC_GLOBALS_H

#include <stdatomic.h>
#include <pthread.h>

extern pthread_mutex_t python_lock;
extern atomic_int active_interpreter_users;
extern atomic_int python_initialized;

#endif
