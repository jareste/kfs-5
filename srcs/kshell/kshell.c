#include "../display/display.h"
#include "../keyboard/keyboard.h"
#include "../utils/utils.h"

static void help();
static void ks_kdump();
static void reboot();

typedef struct
{
    const char* cmd;
    const char* desc;
    void (*func)();
} command_t;

static command_t commands[] = {
    {"help", "Display this help message", help},
    {"exit", "Exit the shell", NULL},
    {"clear", "Clear the screen", clear_screen},
    {"kdump", "Dump memory", ks_kdump},
    {"reboot", "Reboot the system", reboot},
};

static void help()
{
    printf("Available commands:\n");
    
    for (int i = 0; i < sizeof(commands) / sizeof(command_t); i++)
    {
        printf("  %s: %s\n", commands[i].cmd, commands[i].desc);
    }
}

static void ks_kdump()
{
    printf("Enter the address to dump: ");
    clear_kb_buffer();
    while (getc() != 10);
    char* buffer = get_kb_buffer();
    buffer[strlen(buffer) - 1] = '\0'; /* remove '\n' */
    uint32_t addr = (uint32_t)hex_string_to_int(buffer);
    printf("Address: %x\n", addr);
    
    clear_kb_buffer();

    printf("Enter the size to dump: ");
    while (getc() != 10);
    buffer = get_kb_buffer();
    buffer[strlen(buffer) - 1] = '\0'; /* remove '\n' */
    uint32_t size = (uint32_t)hex_string_to_int(buffer);
    printf("Size: %x\n", size);

    if (size < 0 || size > 0x1000)
    {
        printf("Invalid size. Size must be between 0 and 0x1000\n");
        return;
    }

    clear_kb_buffer();
    
    kdump((void*)addr, size);
}

static void reboot()
{
    outb(0x64, 0xFE);
}

void kshell()
{
    int i = 0;

    printf("jareste-OS> ");
    while (1)
    {
        char c = getc();
        if (c == 10)
        {
            char* buffer = get_kb_buffer();
            buffer[strlen(buffer) - 1] = '\0'; /* remove '\n' */
            for (i = 0; i < sizeof(commands) / sizeof(command_t); i++)
            {
                if (strcmp(buffer, commands[i].cmd) == true)
                {
                    if (commands[i].func)
                    {
                        commands[i].func();
                    }
                    else
                    {
                        printf("Not implemented yet\n");
                    }
                    break;
                }
            }
            if (i == sizeof(commands) / sizeof(command_t))
            {
                printf("Command '%s' not found. Type 'help' for a list of available commands\n", buffer);
            }

            clear_kb_buffer();
            printf("jareste-OS> ");
        }
    }
}