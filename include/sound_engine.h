#ifndef SOUND_ENGINE_H
#define SOUND_ENGINE_H

#include "common.h"
#include "event_queue.h"

// Initialize audio engine subsystem
bool sound_engine_init(event_queue_t *queue);

// Stop audio engine worker thread and clean up audio hardware/pipes
void sound_engine_cleanup(void);

// Directly trigger a sound for a given sound_type (useful for testing or fallback)
void sound_engine_play_sound(sound_type_t type);

#endif // SOUND_ENGINE_H
