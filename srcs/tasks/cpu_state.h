#ifndef CPU_STATE_H
#define CPU_STATE_H

#include "../utils/stdint.h"

typedef struct cpu_state
{
    uint32_t edi, esi, ebp, esp_, ebx, edx, ecx, eax;
    uint32_t err_code, int_no;
    uint32_t eip, cs, eflags;
} cpu_state_t;

#endif
