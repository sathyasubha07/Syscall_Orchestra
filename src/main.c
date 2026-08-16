#include "common.h"
#include "process_manager.h"
#include "tracer.h"
#include "decoder.h"
#include "event_queue.h"
#include "sound_engine.h"
#include "signal_handler.h"
#include "display.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

orchestra_config_t g_config;
orchestra_stats_t g_stats;

static void print_usage(const char *prog_name) {
    printf("Syscall Orchestra — Real-Time Linux Syscall Audio Synthesizer\n");
    printf("Usage:\n");
    printf("  %s [options] <target_executable> [target_args...]\n\n", prog_name);
    printf("Options:\n");
    printf("  --no-audio   Disable sound engine (tracing and decoding only)\n");
    printf("  --verbose    Display detailed entry/exit events and register details\n");
    printf("  --debug      Enable PTRACE_PEEKTEXT memory/instruction inspection\n");
    printf("  --quiet      Suppress live event lines, output final stats only\n");
    printf("  --stats      Show summary statistics upon exit\n");
    printf("  --help       Show this help message\n\n");
    printf("Examples:\n");
    printf("  %s ./targets/file_activity\n", prog_name);
    printf("  %s --no-audio ./targets/file_activity\n", prog_name);
    printf("  %s --debug --verbose ./targets/mixed_activity\n", prog_name);
}

int main(int argc, char *argv[]) {
    memset(&g_config, 0, sizeof(g_config));
    memset(&g_stats, 0, sizeof(g_stats));

    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    int target_idx = -1;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--no-audio") == 0) {
            g_config.no_audio = true;
        } else if (strcmp(argv[i], "--verbose") == 0) {
            g_config.verbose = true;
        } else if (strcmp(argv[i], "--debug") == 0) {
            g_config.debug = true;
        } else if (strcmp(argv[i], "--quiet") == 0) {
            g_config.quiet = true;
        } else if (strcmp(argv[i], "--stats") == 0) {
            g_config.stats_only = true;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "[ERROR] Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        } else {
            target_idx = i;
            break;
        }
    }

    if (target_idx == -1) {
        fprintf(stderr, "[ERROR] No target executable specified.\n");
        print_usage(argv[0]);
        return 1;
    }

    g_config.target_path = argv[target_idx];
    g_config.target_argv = &argv[target_idx];
    g_config.target_argc = argc - target_idx;

    // Set up signal handlers for graceful SIGINT/SIGTERM shutdown
    setup_signal_handlers();

    // Initialize decoder and queue
    decoder_init();

    event_queue_t event_q;
    event_queue_init(&event_q);

    // Initialize sound engine
    if (!sound_engine_init(&event_q)) {
        fprintf(stderr, "[WARNING] Sound engine initialization failed; falling back to --no-audio mode.\n");
        g_config.no_audio = true;
    }

    // Launch child process with ptrace TRACEME
    pid_t child_pid = launch_target_process(&g_config);
    if (child_pid <= 0) {
        fprintf(stderr, "[ERROR] Failed to launch target process '%s'.\n", g_config.target_path);
        sound_engine_cleanup();
        event_queue_destroy(&event_q);
        return 1;
    }

    // Run tracing loop
    int res = tracer_run(child_pid, &event_q);

    // Clean up resources
    terminate_child_process(child_pid);
    sound_engine_cleanup();
    event_queue_destroy(&event_q);

    return res;
}
