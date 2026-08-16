#include "sound_engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

/*
 * PHASE 1 STUB
 * ------------
 * The real-time PCM synthesis + ALSA/aplay playback backend is not
 * implemented yet (planned for the next milestone — see docs/architecture.md).
 *
 * This stub keeps the exact same public interface (sound_engine_init /
 * sound_engine_cleanup / sound_engine_play_sound) so the rest of the
 * pipeline (tracer -> decoder -> event_queue) can be built, run, and
 * demonstrated end-to-end right now. It drains the queue on a background
 * thread (so the queue never backs up) but does not synthesize audio.
 */

static pthread_t g_worker_thread;
static event_queue_t *g_queue_ptr = NULL;
static bool g_engine_running = false;

static void *worker_loop(void *arg) {
    (void)arg;
    syscall_event_t ev;
    while (g_engine_running) {
        if (!event_queue_pop(g_queue_ptr, &ev)) {
            break; // queue was shut down
        }
        // NOTE: this is where PCM synthesis + ALSA/aplay output will go.
        // For now we just drain the event so the queue never fills up.
        (void)ev;
    }
    return NULL;
}

bool sound_engine_init(event_queue_t *queue) {
    if (!queue) return false;
    g_queue_ptr = queue;
    g_engine_running = true;

    if (pthread_create(&g_worker_thread, NULL, worker_loop, NULL) != 0) {
        fprintf(stderr, "[ERROR] Sound engine: failed to start worker thread.\n");
        g_engine_running = false;
        return false;
    }

    fprintf(stderr, "[INFO] Sound engine running in STUB mode: queue draining only, "
                     "PCM synthesis/ALSA playback is not implemented yet.\n");
    return true;
}

void sound_engine_cleanup(void) {
    if (!g_engine_running) return;
    g_engine_running = false;
    if (g_queue_ptr) {
        event_queue_shutdown(g_queue_ptr);
    }
    pthread_join(g_worker_thread, NULL);
}

void sound_engine_play_sound(sound_type_t type) {
    (void)type;
    // Not implemented in this build.
}
