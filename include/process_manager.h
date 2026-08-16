#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

#include "common.h"

// Launch the target executable specified in config.
// Returns the child process PID on success, or -1 on failure.
pid_t launch_target_process(const orchestra_config_t *config);

// Terminate child process cleanly if still running
void terminate_child_process(pid_t pid);

#endif // PROCESS_MANAGER_H
