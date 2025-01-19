#include "display/display.h"
#include "keyboard/idt.h"
#include "gdt/gdt.h"
#include "kshell/kshell.h"
#include "timers/timers.h"
#include "memory/memory.h"
#include "memory/mem_utils.h"

extern uint32_t endkernel;

void kernel_main()
{
    clear_screen();
    init_kshell();

    gdt_init();

    init_interrupts();
    paging_init();
    init_timer();

    puts_color(BANNER, LIGHT_GREEN);


    enable_interrupts();
    
    puts("Welcome to the kernel\n");
    kdump((void*)0x1000, 16); // Dump a region of memory
    puts("\n");
    kdump(&endkernel, 0x10);
    printf("Kernel end: %x\n", &endkernel);

    heap_init();

    kshell();
    printf("Exiting shell...\n");
    /* Keep CPU busy */
    while (1)
    {
    }
}
