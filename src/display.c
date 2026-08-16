#include "display.h"
#include "decoder.h"
#include <stdio.h>

#define COLOR_RESET   "\033[0m"
#define COLOR_BOLD    "\033[1m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_RED     "\033[31m"

void display_header(const char *target_path, pid_t pid) {
    if (g_config.quiet) return;

    printf("\n");
    printf(COLOR_BOLD COLOR_CYAN "=========================================================================\n" COLOR_RESET);
    printf(COLOR_BOLD COLOR_MAGENTA "  SYSCALL ORCHESTRA — Turning a Running Program into Music (x86-64 Linux)\n" COLOR_RESET);
    printf(COLOR_BOLD COLOR_CYAN "=========================================================================\n" COLOR_RESET);
    printf("  " COLOR_BOLD "Target Program :" COLOR_RESET " %s\n", target_path);
    printf("  " COLOR_BOLD "Process PID    :" COLOR_RESET " %d\n", pid);
    printf("  " COLOR_BOLD "Audio Engine   :" COLOR_RESET " %s\n", g_config.no_audio ? "DISABLED (--no-audio)" : "ENABLED");
    printf("  " COLOR_BOLD "Tracing Status :" COLOR_RESET " ACTIVE (ptrace entry/exit)\n");
    printf(COLOR_CYAN "-------------------------------------------------------------------------\n" COLOR_RESET);
    printf("\n");
    fflush(stdout);
}

void display_event(const syscall_event_t *ev) {
    if (!ev || g_config.quiet) return;

    const char *cat_color = COLOR_RESET;
    switch (ev->category) {
        case CATEGORY_FILE:    cat_color = COLOR_GREEN; break;
        case CATEGORY_PROCESS: cat_color = COLOR_MAGENTA; break;
        case CATEGORY_MEMORY:  cat_color = COLOR_CYAN; break;
        case CATEGORY_NETWORK: cat_color = COLOR_YELLOW; break;
        case CATEGORY_OTHER:   cat_color = COLOR_BLUE; break;
        default:               cat_color = COLOR_RED; break;
    }

    if (g_config.verbose) {
        printf("[PID %d] %s%-8s%s | %-32s | Sound: %-16s | State: %s\n",
               ev->pid,
               cat_color, category_to_string(ev->category), COLOR_RESET,
               ev->formatted_details,
               ev->sound_name,
               ev->state == EVENT_STATE_ENTRY ? "ENTRY" : "EXIT");
    } else {
        // Standard concise output
        if (ev->state == EVENT_STATE_ENTRY) {
            printf("%-36s  →  Sound: %s%s%s\n", 
                   ev->formatted_details, 
                   cat_color, ev->sound_name, COLOR_RESET);
        } else if (ev->category == CATEGORY_UNKNOWN) {
            printf("%s\n", ev->formatted_details);
        }
    }
    fflush(stdout);
}

void display_debug_peek(pid_t pid, unsigned long addr, unsigned long data) {
    if (!g_config.debug || g_config.quiet) return;
    printf(COLOR_YELLOW "[DEBUG PTRACE_PEEKTEXT] PID %d | RIP Addr: 0x%016lx | Instruction Word: 0x%016lx" COLOR_RESET "\n",
           pid, addr, data);
    fflush(stdout);
}

void display_summary(const orchestra_stats_t *stats) {
    if (!stats) return;

    printf("\n");
    printf(COLOR_BOLD COLOR_CYAN "=========================================================================\n" COLOR_RESET);
    printf(COLOR_BOLD COLOR_GREEN "                      SYSCALL ORCHESTRA SUMMARY                          \n" COLOR_RESET);
    printf(COLOR_BOLD COLOR_CYAN "=========================================================================\n" COLOR_RESET);
    printf("  " COLOR_BOLD "Total Syscalls Captured  :" COLOR_RESET " %lu\n", stats->total_syscalls);
    printf("  " COLOR_BOLD "Recognized Syscalls      :" COLOR_RESET " %lu\n", stats->recognized_syscalls);
    printf("  " COLOR_BOLD "Unknown Syscalls         :" COLOR_RESET " %lu\n", stats->unknown_syscalls);
    printf("  -----------------------------------------------------------------------\n");
    printf("  " COLOR_BOLD "File Syscalls            :" COLOR_RESET " %lu\n", stats->file_syscalls);
    printf("  " COLOR_BOLD "Process Syscalls         :" COLOR_RESET " %lu\n", stats->process_syscalls);
    printf("  " COLOR_BOLD "Memory Syscalls          :" COLOR_RESET " %lu\n", stats->memory_syscalls);
    printf("  " COLOR_BOLD "Network Syscalls         :" COLOR_RESET " %lu\n", stats->network_syscalls);
    printf("  " COLOR_BOLD "Other Syscalls           :" COLOR_RESET " %lu\n", stats->other_syscalls);
    printf("  -----------------------------------------------------------------------\n");
    printf("  " COLOR_BOLD "Sound Events Queued      :" COLOR_RESET " %lu\n", stats->sound_events_queued);
    printf("  " COLOR_BOLD "Sound Events Played      :" COLOR_RESET " %lu\n", stats->sound_events_played);
    printf("  " COLOR_BOLD "Sound Events Dropped     :" COLOR_RESET " %lu\n", stats->sound_events_dropped);
    printf(COLOR_CYAN "=========================================================================\n" COLOR_RESET);
    printf("\n");
    fflush(stdout);
}
