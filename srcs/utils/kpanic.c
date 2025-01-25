#include "../display/display.h"
#include "utils.h"

void kernel_panic_(char* msg, const char* file, int line, const char* func_name)
{
    set_putchar_colour(RED);
    printf("Panic on:\n\tfile: '%s'\n\tline: '%d'\n\tfunc: '%s'\n\tmsg:  '%s'\n",\
    file, line, func_name, msg);
    dump_registers();
    while (1) __asm__ __volatile__("hlt");
}
