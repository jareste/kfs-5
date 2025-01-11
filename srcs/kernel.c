#include "display/display.h"
#include "keyboard/idt.h"
#include "gdt/gdt.h"
#include "kshell/kshell.h"

#define BANNER "Welcome to 42Barcelona's jareste- OS\n"

void kernel_main()
{
    clear_screen();

    gdt_init();

    init_interrupts();

    puts_color(BANNER, LIGHT_GREEN);

    // printf("\n|%x|\n", 0x12345678);

    // put_hex(0x12345678);

    // kdump(&BANNER, strlen(BANNER)); // Dump the memory of the banner

    // kdump((void*)VIDEO_MEMORY, strlen(BANNER)); // Dump the video memory

    enable_interrupts();

    kshell();
    printf("Exiting shell...\n");
    /* Keep CPU busy */
    while (1)
    {
    }
}
