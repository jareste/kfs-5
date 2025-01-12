#include "stdint.h"
uint32_t get_stack_pointer()
{
    uint32_t esp;
    __asm__ __volatile__("mov %%esp, %0" : "=r"(esp));
    return esp;
}
