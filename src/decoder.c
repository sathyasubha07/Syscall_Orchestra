#include "decoder.h"
#include <stdio.h>
#include <string.h>

typedef struct {
    long num;
    const char *name;
    syscall_category_t category;
    sound_type_t sound;
    const char *sound_name;
} syscall_entry_t;

static const syscall_entry_t g_syscall_table[] = {
    // File
    { 0,   "read",       CATEGORY_FILE,    SOUND_LOW_SHORT_NOTE,  "Low Short Note" },
    { 1,   "write",      CATEGORY_FILE,    SOUND_HIGH_SHORT_NOTE, "High Short Note" },
    { 2,   "open",       CATEGORY_FILE,    SOUND_SOFT_CHIME,      "Soft Chime" },
    { 3,   "close",      CATEGORY_FILE,    SOUND_DESCENDING_BLIP, "Descending Blip" },
    { 8,   "lseek",      CATEGORY_FILE,    SOUND_NEUTRAL_BLIP,    "Neutral Blip" },
    { 17,  "pread64",    CATEGORY_FILE,    SOUND_LOW_SHORT_NOTE,  "Low Short Note" },
    { 18,  "pwrite64",   CATEGORY_FILE,    SOUND_HIGH_SHORT_NOTE, "High Short Note" },
    { 19,  "readv",      CATEGORY_FILE,    SOUND_LOW_SHORT_NOTE,  "Low Short Note" },
    { 20,  "writev",     CATEGORY_FILE,    SOUND_HIGH_SHORT_NOTE, "High Short Note" },
    { 22,  "pipe",       CATEGORY_FILE,    SOUND_SOFT_CHIME,      "Soft Chime" },
    { 32,  "dup",        CATEGORY_FILE,    SOUND_NEUTRAL_BLIP,    "Neutral Blip" },
    { 33,  "dup2",       CATEGORY_FILE,    SOUND_NEUTRAL_BLIP,    "Neutral Blip" },
    { 72,  "fcntl",      CATEGORY_FILE,    SOUND_NEUTRAL_BLIP,    "Neutral Blip" },
    { 78,  "getdents",   CATEGORY_FILE,    SOUND_NEUTRAL_BLIP,    "Neutral Blip" },
    { 79,  "getcwd",     CATEGORY_FILE,    SOUND_NEUTRAL_BLIP,    "Neutral Blip" },
    { 80,  "chdir",      CATEGORY_FILE,    SOUND_NEUTRAL_BLIP,    "Neutral Blip" },
    { 83,  "mkdir",      CATEGORY_FILE,    SOUND_SOFT_CHIME,      "Soft Chime" },
    { 84,  "rmdir",      CATEGORY_FILE,    SOUND_DESCENDING_BLIP, "Descending Blip" },
    { 87,  "unlink",     CATEGORY_FILE,    SOUND_DESCENDING_BLIP, "Descending Blip" },
    { 217, "getdents64", CATEGORY_FILE,    SOUND_NEUTRAL_BLIP,    "Neutral Blip" },
    { 257, "openat",     CATEGORY_FILE,    SOUND_SOFT_CHIME,      "Soft Chime" },
    { 293, "pipe2",      CATEGORY_FILE,    SOUND_SOFT_CHIME,      "Soft Chime" },

    // Process
    { 39,  "getpid",     CATEGORY_PROCESS, SOUND_NEUTRAL_BLIP,    "Neutral Blip" },
    { 56,  "clone",      CATEGORY_PROCESS, SOUND_CHORD,           "Chord" },
    { 57,  "fork",       CATEGORY_PROCESS, SOUND_CHORD,           "Chord" },
    { 58,  "vfork",      CATEGORY_PROCESS, SOUND_CHORD,           "Chord" },
    { 59,  "execve",     CATEGORY_PROCESS, SOUND_RISING_SWEEP,    "Rising Sweep" },
    { 60,  "exit",       CATEGORY_PROCESS, SOUND_FADE_OUT_TONE,   "Fade-Out Tone" },
    { 61,  "wait4",      CATEGORY_PROCESS, SOUND_NEUTRAL_BLIP,    "Neutral Blip" },
    { 110, "getppid",    CATEGORY_PROCESS, SOUND_NEUTRAL_BLIP,    "Neutral Blip" },
    { 231, "exit_group", CATEGORY_PROCESS, SOUND_FADE_OUT_TONE,   "Fade-Out Tone" },
    { 247, "waitid",     CATEGORY_PROCESS, SOUND_NEUTRAL_BLIP,    "Neutral Blip" },

    // Memory
    { 9,   "mmap",       CATEGORY_MEMORY,  SOUND_DEEP_BASS,       "Deep Bass" },
    { 10,  "mprotect",   CATEGORY_MEMORY,  SOUND_DEEP_BASS,       "Deep Bass" },
    { 11,  "munmap",     CATEGORY_MEMORY,  SOUND_DEEP_BASS,       "Deep Bass" },
    { 12,  "brk",        CATEGORY_MEMORY,  SOUND_DEEP_BASS,       "Deep Bass" },
    { 25,  "mremap",     CATEGORY_MEMORY,  SOUND_DEEP_BASS,       "Deep Bass" },
    { 28,  "madvise",    CATEGORY_MEMORY,  SOUND_DEEP_BASS,       "Deep Bass" },

    // Network
    { 41,  "socket",     CATEGORY_NETWORK, SOUND_SUSTAINED_TONE,  "Sustained Tone" },
    { 42,  "connect",    CATEGORY_NETWORK, SOUND_SUSTAINED_TONE,  "Sustained Tone" },
    { 43,  "accept",     CATEGORY_NETWORK, SOUND_SUSTAINED_TONE,  "Sustained Tone" },
    { 44,  "sendto",     CATEGORY_NETWORK, SOUND_HIGH_SHORT_NOTE, "High Short Note" },
    { 45,  "recvfrom",   CATEGORY_NETWORK, SOUND_LOW_SHORT_NOTE,  "Low Short Note" },
    { 46,  "sendmsg",    CATEGORY_NETWORK, SOUND_HIGH_SHORT_NOTE, "High Short Note" },
    { 47,  "recvmsg",    CATEGORY_NETWORK, SOUND_LOW_SHORT_NOTE,  "Low Short Note" },
    { 48,  "shutdown",   CATEGORY_NETWORK, SOUND_DESCENDING_BLIP, "Descending Blip" },
    { 49,  "bind",       CATEGORY_NETWORK, SOUND_SUSTAINED_TONE,  "Sustained Tone" },
    { 50,  "listen",     CATEGORY_NETWORK, SOUND_SUSTAINED_TONE,  "Sustained Tone" },
    { 288, "accept4",    CATEGORY_NETWORK, SOUND_SUSTAINED_TONE,  "Sustained Tone" },

    // Other common
    { 7,   "poll",       CATEGORY_OTHER,   SOUND_NEUTRAL_BLIP,    "Neutral Blip" },
    { 13,  "rt_sigaction", CATEGORY_OTHER, SOUND_NEUTRAL_BLIP,    "Neutral Blip" },
    { 14,  "rt_sigprocmask", CATEGORY_OTHER, SOUND_NEUTRAL_BLIP,  "Neutral Blip" },
    { 16,  "ioctl",      CATEGORY_OTHER,   SOUND_NEUTRAL_BLIP,    "Neutral Blip" },
    { 21,  "access",     CATEGORY_OTHER,   SOUND_NEUTRAL_BLIP,    "Neutral Blip" },
    { 23,  "select",     CATEGORY_OTHER,   SOUND_NEUTRAL_BLIP,    "Neutral Blip" },
    { 24,  "sched_yield",CATEGORY_OTHER,   SOUND_NEUTRAL_BLIP,    "Neutral Blip" },
    { 35,  "nanosleep",  CATEGORY_OTHER,   SOUND_NEUTRAL_BLIP,    "Neutral Blip" },
    { 89,  "readlink",   CATEGORY_OTHER,   SOUND_NEUTRAL_BLIP,    "Neutral Blip" },
    { 158, "arch_prctl", CATEGORY_OTHER,   SOUND_NEUTRAL_BLIP,    "Neutral Blip" },
    { 202, "futex",      CATEGORY_OTHER,   SOUND_NEUTRAL_BLIP,    "Neutral Blip" },
    { 230, "clock_nanosleep", CATEGORY_OTHER, SOUND_NEUTRAL_BLIP, "Neutral Blip" }
};

static const size_t g_num_syscalls = sizeof(g_syscall_table) / sizeof(g_syscall_table[0]);

void decoder_init(void) {
    // Nothing complex needed for static table lookup
}

void decode_syscall(long syscall_num, char *name_out, size_t name_size, 
                    syscall_category_t *cat_out, sound_type_t *sound_out, 
                    char *sound_name_out, size_t sound_name_size) {
    for (size_t i = 0; i < g_num_syscalls; i++) {
        if (g_syscall_table[i].num == syscall_num) {
            if (name_out) snprintf(name_out, name_size, "%s", g_syscall_table[i].name);
            if (cat_out) *cat_out = g_syscall_table[i].category;
            if (sound_out) *sound_out = g_syscall_table[i].sound;
            if (sound_name_out) snprintf(sound_name_out, sound_name_size, "%s", g_syscall_table[i].sound_name);
            return;
        }
    }

    // Unknown syscall fallback
    if (name_out) snprintf(name_out, name_size, "unknown (#%ld)", syscall_num);
    if (cat_out) *cat_out = CATEGORY_UNKNOWN;
    if (sound_out) *sound_out = SOUND_UNKNOWN_BLIP;
    if (sound_name_out) snprintf(sound_name_out, sound_name_size, "Unknown Blip");
}

void format_syscall_event(syscall_event_t *ev) {
    if (!ev) return;

    if (ev->state == EVENT_STATE_ENTRY) {
        if (ev->category == CATEGORY_UNKNOWN) {
            snprintf(ev->formatted_details, sizeof(ev->formatted_details), 
                     "[ENTRY] syscall #%ld", ev->syscall_num);
        } else if (strcmp(ev->name, "read") == 0) {
            snprintf(ev->formatted_details, sizeof(ev->formatted_details), 
                     "[ENTRY] read(fd=%d, count=%llu)", 
                     (int)ev->args[0], ev->args[2]);
        } else if (strcmp(ev->name, "write") == 0) {
            snprintf(ev->formatted_details, sizeof(ev->formatted_details), 
                     "[ENTRY] write(fd=%d, count=%llu)", 
                     (int)ev->args[0], ev->args[2]);
        } else if (strcmp(ev->name, "openat") == 0) {
            snprintf(ev->formatted_details, sizeof(ev->formatted_details), 
                     "[ENTRY] openat(dfd=%d, flags=0x%llx)", 
                     (int)ev->args[0], ev->args[2]);
        } else if (strcmp(ev->name, "open") == 0) {
            snprintf(ev->formatted_details, sizeof(ev->formatted_details), 
                     "[ENTRY] open(flags=0x%llx)", ev->args[1]);
        } else if (strcmp(ev->name, "close") == 0) {
            snprintf(ev->formatted_details, sizeof(ev->formatted_details), 
                     "[ENTRY] close(fd=%d)", (int)ev->args[0]);
        } else if (strcmp(ev->name, "mmap") == 0) {
            snprintf(ev->formatted_details, sizeof(ev->formatted_details), 
                     "[ENTRY] mmap(len=%llu, prot=0x%llx, fd=%d)", 
                     ev->args[1], ev->args[2], (int)ev->args[4]);
        } else if (strcmp(ev->name, "brk") == 0) {
            snprintf(ev->formatted_details, sizeof(ev->formatted_details), 
                     "[ENTRY] brk(addr=0x%llx)", ev->args[0]);
        } else if (strcmp(ev->name, "socket") == 0) {
            snprintf(ev->formatted_details, sizeof(ev->formatted_details), 
                     "[ENTRY] socket(domain=%d, type=%d, proto=%d)", 
                     (int)ev->args[0], (int)ev->args[1], (int)ev->args[2]);
        } else if (strcmp(ev->name, "connect") == 0) {
            snprintf(ev->formatted_details, sizeof(ev->formatted_details), 
                     "[ENTRY] connect(fd=%d, addrlen=%d)", 
                     (int)ev->args[0], (int)ev->args[2]);
        } else {
            snprintf(ev->formatted_details, sizeof(ev->formatted_details), 
                     "[ENTRY] %s(arg0=0x%llx, arg1=0x%llx)", 
                     ev->name, ev->args[0], ev->args[1]);
        }
    } else {
        // EXIT STATE
        if (ev->category == CATEGORY_UNKNOWN) {
            snprintf(ev->formatted_details, sizeof(ev->formatted_details), 
                     "[EXIT ] syscall #%ld → %ld", ev->syscall_num, ev->retval);
        } else {
            snprintf(ev->formatted_details, sizeof(ev->formatted_details), 
                     "[EXIT ] %s → %ld", ev->name, ev->retval);
        }
    }
}

const char* category_to_string(syscall_category_t cat) {
    switch (cat) {
        case CATEGORY_FILE:    return "FILE";
        case CATEGORY_PROCESS: return "PROCESS";
        case CATEGORY_MEMORY:  return "MEMORY";
        case CATEGORY_NETWORK: return "NETWORK";
        case CATEGORY_OTHER:   return "OTHER";
        default:               return "UNKNOWN";
    }
}

const char* sound_type_to_string(sound_type_t sound) {
    switch (sound) {
        case SOUND_LOW_SHORT_NOTE:  return "Low Short Note";
        case SOUND_HIGH_SHORT_NOTE: return "High Short Note";
        case SOUND_SOFT_CHIME:      return "Soft Chime";
        case SOUND_DESCENDING_BLIP: return "Descending Blip";
        case SOUND_CHORD:           return "Chord";
        case SOUND_RISING_SWEEP:    return "Rising Sweep";
        case SOUND_SUSTAINED_TONE:  return "Sustained Tone";
        case SOUND_DEEP_BASS:       return "Deep Bass";
        case SOUND_FADE_OUT_TONE:   return "Fade-Out Tone";
        case SOUND_NEUTRAL_BLIP:    return "Neutral Blip";
        case SOUND_UNKNOWN_BLIP:    return "Unknown Blip";
        default:                    return "None";
    }
}
