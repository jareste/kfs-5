#include "../display/display.h"
#include "../keyboard/keyboard.h"

/* must implement getc() */

typedef struct
{
    const char* cmd;
    const char* desc;
    void (*func)();
} command_t;

static void help()
{
    printf("Available commands:\n");
    printf("  help: Display this help message\n");
    printf("  exit: Exit the shell\n");
    printf("  clear: Clear the screen\n");
    printf("  kdump: Dump memory\n");
    printf("  reboot: Reboot the system\n");
}

void kshell()
{
    command_t commands[] = {
        {"help", "Display this help message", help},
        {"exit", "Exit the shell", NULL},
        {"clear", "Clear the screen", clear_screen},
        {"kdump", "Dump memory", NULL},
        {"reboot", "Reboot the system", NULL},
    };
 
    printf("jareste-OS> ");
    while (1)
    {
        char c = getc();
        if (c == 10)
        {
            char* buffer = get_kb_buffer();
            buffer[strlen(buffer) - 1] = '\0';
            for (int i = 0; i < sizeof(commands) / sizeof(command_t); i++)
            {
                if (strcmp(buffer, commands[i].cmd) == true)
                {
                    if (commands[i].func)
                    {
                        commands[i].func();
                    }
                    break;
                }
            }
            
            // if (strncmp(buffer, "exit", 5) == true)
            // {
            //     break;
            // }
            // else if (strncmp(buffer, "help", 5) == true)
            // {
            //     printf("Available commands:\n");
            //     printf("  help: Display this help message\n");
            //     printf("  exit: Exit the shell\n");
            // }
            // else if (strncmp(buffer, "clear", 5) == true)
            // {
            //     clear_screen();
            // }
            // else if (strncmp(buffer, "kdump", 4) == true)
            // {
            //     kdump((void*)VIDEO_MEMORY, 50);
            // }
            // else if (strncmp(buffer, "reboot", 6) == true)
            // {
            //     outb(0x64, 0xFE);
            // }
            // else
            // {
            //     printf("Unknown command: '%s'. Try 'help' for more info.\n", buffer);
            // }
            clear_kb_buffer();
            printf("jareste-OS> ");
        }
    }
}