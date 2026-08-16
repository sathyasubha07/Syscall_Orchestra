#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

#include <signal.h>
#include <stdbool.h>

extern volatile sig_atomic_t g_shutdown_requested;

// Register signal handlers for SIGINT and SIGTERM
void setup_signal_handlers(void);

// Check if shutdown was requested
bool is_shutdown_requested(void);

#endif // SIGNAL_HANDLER_H
