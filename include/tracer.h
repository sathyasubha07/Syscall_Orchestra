#ifndef TRACER_H
#define TRACER_H

#include "common.h"
#include "event_queue.h"

// Run ptrace tracing loop on target_pid until process exit or signal
int tracer_run(pid_t target_pid, event_queue_t *queue);

#endif // TRACER_H
