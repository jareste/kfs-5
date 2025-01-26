#include "display/display.h"
#include "keyboard/idt.h"
#include "gdt/gdt.h"
#include "kshell/kshell.h"
#include "timers/timers.h"
#include "memory/memory.h"
#include "keyboard/signals.h"

extern uint32_t endkernel;

void kernel_main()
{
    clear_screen();
    init_kshell();

    gdt_init();

    init_interrupts();

    paging_init();

    init_timer();

    init_signals();
    enable_interrupts();

    heap_init();

    kshell();
    printf("Exiting shell...\n");
    /* Keep CPU busy */
    while (1)
    {
    }
}
