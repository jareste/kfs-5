#ifndef UTILS_H
#define UTILS_H

#include "stdint.h"

typedef enum bool {
    false,
    true
} bool;

int strlen(const char *str);
bool strcmp(const char *str1, const char *str2);
void itoa(int value, char *str, int base);

#endif
