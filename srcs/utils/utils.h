#ifndef UTILS_H
#define UTILS_H

#include "stdint.h"
#define NULL (void*)0
#define UNUSED(x); (void)(x);

typedef enum bool {
    false = 0,
    true = 1
} bool;

typedef struct {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t esi;
    uint32_t edi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t eip;
    uint32_t eflags;
    uint32_t cs;
    uint32_t ds;
    uint32_t es;
    uint32_t fs;
    uint32_t gs;
    uint32_t ss;
} registers_t;

int strlen(const char *str);
bool strcmp(const char *str1, const char *str2);
void itoa(int value, char *str, int base);
void memset(void *dest, uint8_t val, uint32_t len);
bool strncmp(const char *str1, const char *str2, int n);
uint32_t strtol(const char* str, char** endptr, int base);
uint32_t hex_string_to_int(const char *hex_str);
uint32_t get_stack_pointer();
void dump_registers_c(registers_t* regs);
void kernel_panic();
extern void dump_registers();

#endif
