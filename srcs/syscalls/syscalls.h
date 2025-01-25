#ifndef SYSCALLS_H
#define SYSCALLS_H
#include "../keyboard/idt.h"

#define SYS_EXIT    0
#define SYS_WRITE   1
#define SYS_READ    2
#define SYS_OPEN    3
#define SYS_CLOSE   4
#define SYS_GETPID  5

void syscall_handler(registers reg, uint32_t intr_no, uint32_t err_code, error_state stack);

#endif // SYSCALLS_H