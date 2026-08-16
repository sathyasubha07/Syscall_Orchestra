#ifndef DECODER_H
#define DECODER_H

#include "common.h"

// Initialize decoder data structures
void decoder_init(void);

// Decode syscall number to name, category, and sound type
void decode_syscall(long syscall_num, char *name_out, size_t name_size, 
                    syscall_category_t *cat_out, sound_type_t *sound_out, 
                    char *sound_name_out, size_t sound_name_size);

// Format syscall entry/exit details into string
void format_syscall_event(syscall_event_t *event);

// Convert category enum to human readable string
const char* category_to_string(syscall_category_t cat);

// Convert sound type enum to human readable string
const char* sound_type_to_string(sound_type_t sound);

#endif // DECODER_H
