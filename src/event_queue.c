#include "event_queue.h"
#include <stdio.h>
#include <string.h>

void event_queue_init(event_queue_t *q) {
    if (!q) return;
    q->head = 0;
    q->tail = 0;
    q->count = 0;
    q->shutdown = false;
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->cond_not_empty, NULL);
    pthread_cond_init(&q->cond_not_full, NULL);
}

void event_queue_destroy(event_queue_t *q) {
    if (!q) return;
    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy(&q->cond_not_empty);
    pthread_cond_destroy(&q->cond_not_full);
}

bool event_queue_push(event_queue_t *q, const syscall_event_t *ev) {
    if (!q || !ev) return false;

    pthread_mutex_lock(&q->lock);

    if (q->shutdown) {
        pthread_mutex_unlock(&q->lock);
        return false;
    }

    if (q->count >= EVENT_QUEUE_CAPACITY) {
        // Queue overflow throttling: drop event so tracer never blocks
        g_stats.sound_events_dropped++;
        pthread_mutex_unlock(&q->lock);
        return false;
    }

    q->events[q->tail] = *ev;
    q->tail = (q->tail + 1) % EVENT_QUEUE_CAPACITY;
    q->count++;
    g_stats.sound_events_queued++;

    pthread_cond_signal(&q->cond_not_empty);
    pthread_mutex_unlock(&q->lock);
    return true;
}

bool event_queue_pop(event_queue_t *q, syscall_event_t *ev_out) {
    if (!q || !ev_out) return false;

    pthread_mutex_lock(&q->lock);

    while (q->count == 0 && !q->shutdown) {
        pthread_cond_wait(&q->cond_not_empty, &q->lock);
    }

    if (q->shutdown && q->count == 0) {
        pthread_mutex_unlock(&q->lock);
        return false;
    }

    *ev_out = q->events[q->head];
    q->head = (q->head + 1) % EVENT_QUEUE_CAPACITY;
    q->count--;

    pthread_cond_signal(&q->cond_not_full);
    pthread_mutex_unlock(&q->lock);
    return true;
}

void event_queue_shutdown(event_queue_t *q) {
    if (!q) return;

    pthread_mutex_lock(&q->lock);
    q->shutdown = true;
    pthread_cond_broadcast(&q->cond_not_empty);
    pthread_cond_broadcast(&q->cond_not_full);
    pthread_mutex_unlock(&q->lock);
}
