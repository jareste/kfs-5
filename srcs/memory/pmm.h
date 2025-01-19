#ifndef PMM_H
#define PMM_H

#include "../utils/stdint.h"

void pmm_init();
uint32_t allocate_frame();
void free_frame(uint32_t phys_addr);

#endif