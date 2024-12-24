#include "display/display.h"

#define BANNER "Welcome to 42Barcelona's jareste- OS\n"

void kernel_main()
{
    clear_screen();
    // init_interrupts();

    puts_color(BANNER, LIGHT_GREEN);
  
    

    /* Keep CPU busy */
    while (1)
    {
    }
}


