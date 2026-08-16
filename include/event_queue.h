#ifndef EVENT_QUEUE_H
#define EVENT_QUEUE_H

#include "common.h"
#include <pthread.h>

#define EVENT_QUEUE_CAPACITY 256

typedef struct {
    syscall_event_t events[EVENT_QUEUE_CAPACITY];
    int head;
    int tail;
    int count;
    pthread_mutex_t lock;
    pthread_cond_t cond_not_empty;
    pthread_cond_t cond_not_full;
    bool shutdown;
} event_queue_t;

// Initialize event queue
void event_queue_init(event_queue_t *q);

// Destroy event queue and free resources
void event_queue_destroy(event_queue_t *q);

// Push event to queue (non-blocking if full: drops event with stat bump)
bool event_queue_push(event_queue_t *q, const syscall_event_t *ev);

// Pop event from queue (blocking until item available or shutdown)
bool event_queue_pop(event_queue_t *q, syscall_event_t *ev_out);

// Wake up waiting consumers and mark queue as shut down
void event_queue_shutdown(event_queue_t *q);

#endif // EVENT_QUEUE_H
