#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#define MAX_SYSCALL_NAME 64
#define MAX_ARGS 6
#define MAX_SOUND_NAME 32
#define MAX_FORMATTED_LEN 256

// Category classification for syscalls
typedef enum {
    CATEGORY_FILE = 0,
    CATEGORY_PROCESS,
    CATEGORY_MEMORY,
    CATEGORY_NETWORK,
    CATEGORY_OTHER,
    CATEGORY_UNKNOWN
} syscall_category_t;

// State of syscall event
typedef enum {
    EVENT_STATE_ENTRY = 0,
    EVENT_STATE_EXIT
} event_state_t;

// Sound type enumeration
typedef enum {
    SOUND_NONE = 0,
    SOUND_LOW_SHORT_NOTE,   // read
    SOUND_HIGH_SHORT_NOTE,  // write
    SOUND_SOFT_CHIME,       // open, openat
    SOUND_DESCENDING_BLIP,  // close
    SOUND_CHORD,            // clone, fork, vfork
    SOUND_RISING_SWEEP,     // execve
    SOUND_SUSTAINED_TONE,   // connect, socket, bind, listen
    SOUND_DEEP_BASS,        // mmap, brk, munmap, mprotect
    SOUND_FADE_OUT_TONE,    // exit, exit_group
    SOUND_NEUTRAL_BLIP,     // sendto, recvfrom, select, poll, etc.
    SOUND_UNKNOWN_BLIP      // unknown syscalls
} sound_type_t;

// Structure holding a single syscall event
typedef struct {
    pid_t pid;
    long syscall_num;
    char name[MAX_SYSCALL_NAME];
    syscall_category_t category;
    unsigned long long args[MAX_ARGS];
    long retval;
    event_state_t state;
    sound_type_t sound;
    char sound_name[MAX_SOUND_NAME];
    char formatted_details[MAX_FORMATTED_LEN];
    double timestamp;
    unsigned long peek_word; // Content read via PTRACE_PEEKTEXT if debug enabled
    bool peek_valid;
} syscall_event_t;

// Global CLI Configuration settings
typedef struct {
    bool no_audio;
    bool verbose;
    bool debug;
    bool quiet;
    bool stats_only;
    const char *target_path;
    char **target_argv;
    int target_argc;
} orchestra_config_t;

// Statistics counter structure
typedef struct {
    unsigned long total_syscalls;
    unsigned long recognized_syscalls;
    unsigned long unknown_syscalls;
    unsigned long file_syscalls;
    unsigned long process_syscalls;
    unsigned long memory_syscalls;
    unsigned long network_syscalls;
    unsigned long other_syscalls;
    unsigned long sound_events_queued;
    unsigned long sound_events_played;
    unsigned long sound_events_dropped;
} orchestra_stats_t;

extern orchestra_config_t g_config;
extern orchestra_stats_t g_stats;

#endif // COMMON_H
