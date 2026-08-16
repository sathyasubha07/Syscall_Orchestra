#include "signal_handler.h"
#include <stdio.h>
#include <string.h>

volatile sig_atomic_t g_shutdown_requested = 0;

static void handle_signal(int sig) {
    (void)sig;
    g_shutdown_requested = 1;
}

void setup_signal_handlers(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    // Ignore SIGPIPE so the application doesn't terminate if the audio pipe (aplay) breaks
    signal(SIGPIPE, SIG_IGN);
}

bool is_shutdown_requested(void) {
    return g_shutdown_requested != 0;
}
