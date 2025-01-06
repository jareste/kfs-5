#include "display/display.h"
#include "keyboard/idt.h"

#define BANNER "Welcome to 42Barcelona's jareste- OS\n"

void kernel_main()
{
    clear_screen();

    init_interrupts();

    puts_color(BANNER, LIGHT_GREEN);


    enable_interrupts();

    /* Keep CPU busy */
    while (1)
    {
    }
}
