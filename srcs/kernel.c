#include "display/display.h"
#include "keyboard/idt.h"
#include "gdt/gdt.h"
#include "kshell/kshell.h"
#include "timers/timers.h"
#include "memory/memory.h"

extern uint32_t endkernel;

void kernel_main()
{
    clear_screen();

    gdt_init();

    init_interrupts();
    init_timer();

    puts_color(BANNER, LIGHT_GREEN);

    // printf("\n|%x|\n", 0x12345678);

    // put_hex(0x12345678);

    // kdump(&BANNER, strlen(BANNER)); // Dump the memory of the banner

    // kdump((void*)VIDEO_MEMORY, strlen(BANNER)); // Dump the video memory

    enable_interrupts();
    // init_paging();

    // void*test = kmalloc(0x1000);
    // if (test == NULL)
    // {
    //     puts("kmalloc failed\n");
    // }

    // test = kmalloc(0x1000);
    // if (test == NULL)
    // {
    //     puts("kmalloc failed\n");
    // }
    // test = kmalloc(0x1000);
    // if (test == NULL)
    // {
    //     puts("kmalloc failed\n");
    // }
    // test = kmalloc(0x1000);
    // if (test == NULL)
    // {
    //     puts("kmalloc failed\n");
    // }
    // test = kmalloc(0x1000);
    // if (test == NULL)
    // {
    //     puts("kmalloc failed\n");
    // }
    // test = kmalloc(0x1000);
    // if (test == NULL)
    // {
    //     puts("kmalloc failed\n");
    // }
    // test = kmalloc(0x1000);
    // if (test == NULL)
    // {
    //     puts("kmalloc failed\n");
    // }

    kdump(&endkernel, 0x10);
    printf("Kernel end: %x\n", &endkernel);

    kshell();
    printf("Exiting shell...\n");
    /* Keep CPU busy */
    while (1)
    {
    }
}
