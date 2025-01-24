#include "../display/display.h"
#include "utils.h"

void kernel_panic(char* msg)
{
    set_putchar_colour(RED);
    puts("Kernel panic: ");
    puts(msg);
    putc('\n');
    dump_registers();
    while (1) __asm__ __volatile__("hlt");
}
