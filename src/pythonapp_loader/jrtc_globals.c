#include <stdatomic.h>
#include <pthread.h>
#include "jrtc_globals.h"

pthread_mutex_t python_lock = PTHREAD_MUTEX_INITIALIZER;
atomic_int active_interpreter_users = 0;
atomic_int python_initialized = 0;
