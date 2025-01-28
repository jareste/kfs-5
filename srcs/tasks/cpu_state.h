#ifndef CPU_STATE_H
#define CPU_STATE_H

#include "../utils/stdint.h"

typedef struct cpu_state
{
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t esi;
    uint32_t edi;
    uint32_t ebp;
    uint32_t esp_;
    uint32_t eip;
    uint32_t eflags;
} cpu_state_t;


#endif
