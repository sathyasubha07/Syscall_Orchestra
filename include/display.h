#ifndef DISPLAY_H
#define DISPLAY_H

#include "common.h"

// Print application header banner
void display_header(const char *target_path, pid_t pid);

// Render live syscall event to console based on verbosity/quiet settings
void display_event(const syscall_event_t *ev);

// Display PTRACE_PEEKTEXT debug details
void display_debug_peek(pid_t pid, unsigned long addr, unsigned long data);

// Display final statistics summary table
void display_summary(const orchestra_stats_t *stats);

#endif // DISPLAY_H
