#ifndef STDLIB_H
#define STDLIB_H
#include "../utils/stdint.h"

int write(int fd, const char* buf, size_t count);
int kill(uint32_t pid, uint32_t signal);
int signal(uint32_t pid, uint32_t signal);
size_t read(int fd, char* buf, size_t count);

#endif
