#ifndef GDT_H
#define GDT_H

#include <stdint.h>

/* https://wiki.osdev.org/GDT_Tutorial */
#define GDT_ENTRIES 7
#define GDT_ADDRESS 0x00000800

typedef struct __attribute__((packed)) {
    uint16_t limit_low;   /* Lower 16 bits of segment limit */
    uint16_t base_low;    /* Lower 16 bits of base address */
    uint8_t base_middle;  /* Next 8 bits of base address */
    uint8_t access;       /* Access flags (e.g., type, privilege level) */
    uint8_t granularity;  /* Granularity and upper limit bits */
    uint8_t base_high;    /* Final 8 bits of base address */
} gdt_entry_t;

typedef struct __attribute__((packed)) {
    uint16_t limit;
    uint32_t base;
} gdt_ptr_t;

void gdt_init();

#endif
