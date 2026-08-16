#include "tracer.h"
#include "decoder.h"
#include "display.h"
#include "signal_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>

int tracer_run(pid_t target_pid, event_queue_t *queue) {
    if (target_pid <= 0) return -1;

    int status = 0;

    // 1. Wait for child initial stop (from PTRACE_TRACEME + execve)
    if (waitpid(target_pid, &status, 0) < 0) {
        perror("[ERROR] Tracer: Initial waitpid failed");
        return -1;
    }

    if (!WIFSTOPPED(status)) {
        fprintf(stderr, "[ERROR] Tracer: Child did not stop as expected on initial startup.\n");
        return -1;
    }

    // Set PTRACE_O_TRACESYSGOOD so syscall stops deliver (SIGTRAP | 0x80)
    if (ptrace(PTRACE_SETOPTIONS, target_pid, 0, PTRACE_O_TRACESYSGOOD) < 0) {
        // Fallback: continue even if PTRACE_O_TRACESYSGOOD is unsupported
    }

    display_header(g_config.target_path, target_pid);

    bool is_entry = true;

    // Tell child to continue to next syscall entry
    if (ptrace(PTRACE_SYSCALL, target_pid, 0, 0) < 0) {
        perror("[ERROR] Tracer: Initial ptrace(PTRACE_SYSCALL) failed");
        return -1;
    }

    // Main waitpid / ptrace event loop
    while (!is_shutdown_requested()) {
        pid_t wpid = waitpid(target_pid, &status, 0);

        if (wpid < 0) {
            if (errno == EINTR) continue;
            perror("[ERROR] Tracer: waitpid failed");
            break;
        }

        // Check if process exited normally
        if (WIFEXITED(status)) {
            int exit_code = WEXITSTATUS(status);
            if (!g_config.quiet) {
                printf("\n[INFO] Target Process (PID %d) exited normally with code %d.\n", 
                       target_pid, exit_code);
            }
            break;
        }

        // Check if process terminated by unhandled signal
        if (WIFSIGNALED(status)) {
            int sig = WTERMSIG(status);
            if (!g_config.quiet) {
                printf("\n[WARNING] Target Process (PID %d) terminated by signal %d (%s).\n", 
                       target_pid, sig, strsignal(sig));
            }
            break;
        }

        // Check if process stopped
        if (WIFSTOPPED(status)) {
            int stopsig = WSTOPSIG(status);

            // Syscall stop detected (either SIGTRAP | 0x80 or SIGTRAP)
            if (stopsig == (SIGTRAP | 0x80) || stopsig == SIGTRAP) {
                struct user_regs_struct regs;
                if (ptrace(PTRACE_GETREGS, target_pid, 0, &regs) < 0) {
                    perror("[ERROR] Tracer: ptrace(PTRACE_GETREGS) failed");
                    break;
                }

                syscall_event_t ev;
                memset(&ev, 0, sizeof(ev));
                ev.pid = target_pid;
                ev.syscall_num = regs.orig_rax;
                ev.args[0] = regs.rdi;
                ev.args[1] = regs.rsi;
                ev.args[2] = regs.rdx;
                ev.args[3] = regs.r10;
                ev.args[4] = regs.r8;
                ev.args[5] = regs.r9;
                ev.retval = regs.rax;

                decode_syscall(ev.syscall_num, ev.name, sizeof(ev.name), 
                               &ev.category, &ev.sound, ev.sound_name, sizeof(ev.sound_name));

                if (is_entry) {
                    ev.state = EVENT_STATE_ENTRY;
                    g_stats.total_syscalls++;

                    if (ev.category == CATEGORY_UNKNOWN) {
                        g_stats.unknown_syscalls++;
                    } else {
                        g_stats.recognized_syscalls++;
                    }

                    switch (ev.category) {
                        case CATEGORY_FILE:    g_stats.file_syscalls++; break;
                        case CATEGORY_PROCESS: g_stats.process_syscalls++; break;
                        case CATEGORY_MEMORY:  g_stats.memory_syscalls++; break;
                        case CATEGORY_NETWORK: g_stats.network_syscalls++; break;
                        default:               g_stats.other_syscalls++; break;
                    }

                    // Optional PTRACE_PEEKTEXT inspection under --debug
                    if (g_config.debug) {
                        errno = 0;
                        long peek_val = ptrace(PTRACE_PEEKTEXT, target_pid, (void*)regs.rip, NULL);
                        if (errno == 0) {
                            ev.peek_word = (unsigned long)peek_val;
                            ev.peek_valid = true;
                            display_debug_peek(target_pid, (unsigned long)regs.rip, ev.peek_word);
                        } else {
                            ev.peek_valid = false;
                        }
                    }

                    format_syscall_event(&ev);
                    display_event(&ev);

                    // Push event to audio queue (once per syscall at entry)
                    if (queue && !g_config.no_audio) {
                        event_queue_push(queue, &ev);
                    }

                    is_entry = false;
                } else {
                    ev.state = EVENT_STATE_EXIT;
                    format_syscall_event(&ev);
                    if (g_config.verbose || ev.category == CATEGORY_UNKNOWN) {
                        display_event(&ev);
                    }

                    is_entry = true;
                }

                // Continue target process to next syscall stop
                if (ptrace(PTRACE_SYSCALL, target_pid, 0, 0) < 0) {
                    perror("[ERROR] Tracer: ptrace(PTRACE_SYSCALL) failed");
                    break;
                }
            } else {
                // Stopped by another signal (pass signal to target process)
                if (ptrace(PTRACE_SYSCALL, target_pid, 0, (void*)(long)stopsig) < 0) {
                    perror("[ERROR] Tracer: ptrace(PTRACE_SYSCALL with signal) failed");
                    break;
                }
            }
        }
    }

    display_summary(&g_stats);
    return 0;
}
