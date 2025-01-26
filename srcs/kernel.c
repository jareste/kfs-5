#include "display/display.h"
#include "keyboard/idt.h"
#include "gdt/gdt.h"
#include "kshell/kshell.h"
#include "timers/timers.h"
#include "memory/memory.h"
#include "keyboard/signals.h"
#include "tasks/task.h"

extern uint32_t endkernel;

void kernel_main()
{
    clear_screen();
    init_kshell();

    paging_init();
    init_interrupts();
    heap_init();
    gdt_init();

    init_timer();

    init_signals();


    init_tasks();
    enable_interrupts();

    kshell();
    printf("Exiting shell...\n");
    /* Keep CPU busy */
    while (1)
    {
    }
}
