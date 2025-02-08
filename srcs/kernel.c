#include "display/display.h"
#include "keyboard/idt.h"
#include "gdt/gdt.h"
#include "kshell/kshell.h"
#include "timers/timers.h"
#include "memory/memory.h"
#include "keyboard/signals.h"
#include "tasks/task.h"
#include "ide/ide.h"
#include "ide/ext2.h"

#include "umgmnt/users.h"

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

    enable_interrupts();

    ide_demo();

    ext2_mount();
    ext2_demo();

    user_t root;
    root = find_user_by_name("root");
    if (root.is_valid)
    {
        printf("User 'root' found!\n");
        printf("User: %s\n", root.name);
        printf("UID: %d\n", root.uid);
        printf("GID: %d\n", root.gid);
        printf("Home inode: %d\n", root.home_inode);
        printf("Pass: %s\n", root.pass_hash);
        if (check_password("123456", root.pass_hash))
        {
            printf("Password correct!\n");
        }
        else
        {
            printf("Password incorrect!\n");
        }
    }
    else
    {
        printf("User 'root' not found!\n");
    }
    
    scheduler_init();
    start_foo_tasks();
    scheduler();

    while (1)
    {
    }
    kshell();
    printf("Exiting shell...\n");
    /* Keep CPU busy */
    
}
