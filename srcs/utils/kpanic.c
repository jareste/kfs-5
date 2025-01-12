#include "../display/display.h"
#include "utils.h"

void kernel_panic()
{
    printf("Kernel Panic: Dumping Registers\n");
    dump_registers();
    while (1) __asm__ __volatile__("hlt");
}
