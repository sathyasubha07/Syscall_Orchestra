#include "process_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>

extern char **environ;

pid_t launch_target_process(const orchestra_config_t *config) {
    if (!config || !config->target_path) {
        fprintf(stderr, "[ERROR] Process Manager: Invalid target executable path.\n");
        return -1;
    }

    // Check target file access
    if (access(config->target_path, X_OK) != 0) {
        fprintf(stderr, "[ERROR] Process Manager: Executable '%s' not found or not executable: %s\n", 
                config->target_path, strerror(errno));
        return -1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("[ERROR] Process Manager: fork failed");
        return -1;
    }

    if (pid == 0) {
        // Child Process
        // Allow parent process to trace this child
        if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) < 0) {
            perror("[ERROR] Child: ptrace(PTRACE_TRACEME) failed");
            _exit(1);
        }

        // Execute the target program
        execve(config->target_path, config->target_argv, environ);

        // If execve returns, an error occurred
        fprintf(stderr, "[ERROR] Child: execve failed for '%s': %s\n", 
                config->target_path, strerror(errno));
        _exit(127);
    }

    // Parent Process
    return pid;
}

void terminate_child_process(pid_t pid) {
    if (pid <= 0) return;

    // Send SIGTERM first
    if (kill(pid, SIGTERM) == 0) {
        usleep(50000); // 50ms grace period
        int status;
        if (waitpid(pid, &status, WNOHANG) == 0) {
            // Still alive, send SIGKILL
            kill(pid, SIGKILL);
            waitpid(pid, &status, 0);
        }
    } else {
        // Process might already be dead, reap it
        int status;
        waitpid(pid, &status, WNOHANG);
    }
}
