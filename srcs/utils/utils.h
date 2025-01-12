#ifndef UTILS_H
#define UTILS_H

#include "stdint.h"
#define NULL 0

typedef enum bool {
    false = 0,
    true = 1
} bool;

int strlen(const char *str);
bool strcmp(const char *str1, const char *str2);
void itoa(int value, char *str, int base);
void memset(void *dest, uint8_t val, uint32_t len);
bool strncmp(const char *str1, const char *str2, int n);
uint32_t strtol(const char* str, char** endptr, int base);
uint32_t hex_string_to_int(const char *hex_str);
uint32_t get_stack_pointer();

#endif
