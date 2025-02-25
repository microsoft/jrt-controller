#define _GNU_SOURCE
#include <linux/sched.h>
#include <stdio.h>
#include <unistd.h>

#include "jrtc_sched.h"
#include "jrtc_logging.h"

static int
sched_setattr(pid_t pid, const struct sched_attr* attr, unsigned int flags)
{
    return syscall(__NR_sched_setattr, pid, attr, flags);
}

int
jrtc_thread_set_scheduler(pthread_t tid, struct jrtc_sched_config* sched_config)
{

    int res;

    if (!sched_config)
        return -1;

    switch (sched_config->sched_policy) {
    case JRTC_SCHED_NORMAL:
        jrtc_logger(JRTC_WARN, "Using normal scheduling policy\n");
        res = 0;
        break;

    case JRTC_SCHED_FIFO: {
        int policy = SCHED_FIFO;
        struct sched_param param;
        param.sched_priority = sched_config->sched_priority;
        jrtc_logger(
            JRTC_WARN, "Setting router scheduling policy to SCHED_FIFO, with priority %d\n", param.sched_priority);
        res = pthread_setschedparam(tid, policy, &param);
        break;
    }

    case JRTC_SCHED_DEADLINE: {
        struct sched_attr attr;
        attr.size = sizeof(attr);
        attr.sched_flags = 0;
        attr.sched_nice = 0;
        attr.sched_priority = 0;
        attr.sched_policy = SCHED_DEADLINE;
        attr.sched_runtime = sched_config->sched_runtime_us * 1000;
        attr.sched_period = sched_config->sched_period_us * 1000;
        attr.sched_deadline = sched_config->sched_deadline_us * 1000;
        jrtc_logger(
            JRTC_WARN,
            "Setting router scheduling policy to SCHED_DEADLINE, with runtime %lld, period %lld and deadline %lld\n",
            attr.sched_runtime,
            attr.sched_period,
            attr.sched_deadline);
        res = sched_setattr(0, &attr, 0);
        break;
    }

    default:
        jrtc_logger(JRTC_ERROR, "Unknown router scheduling policy\n");
        res = -2;
        break;
    }

    if (res == 0) {
        jrtc_logger(JRTC_INFO, "Scheduling policy applied successfully\n");
    } else {
        jrtc_logger(JRTC_ERROR, "Error setting the scheduling policy of the router\n");
    }
    return res;
}
