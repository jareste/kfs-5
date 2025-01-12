#include "../display/display.h"
#include "../keyboard/keyboard.h"
#include "../utils/utils.h"

static void help();
static void ks_kdump();
static void reboot();
static void kdump_stack();
static void halt();
static void hhalt();
static void colour();

typedef struct
{
    const char* cmd;
    const char* desc;
    void (*func)();
    bool hidden;
} command_t;

static command_t commands[] = {
    {"help", "Display this help message", help, false},
    {"?", "", help, true},
    {"h", "", help, true},
    {"exit", "Exit the shell", NULL, false},
    {"clear", "Clear the screen", clear_screen, false},
    {"kdump", "Dump memory", ks_kdump, false},
    {"stack", "Dump stack", kdump_stack, false},
    {"reboot", "Reboot the system", reboot, false},
    {"halt", "Halt the system.", halt, false},
    {"sudo halt", "Halt the system. (Stops it).", hhalt, true},
    {"colour", "Set shell colour", colour, false},
};

static void colour()
{
    printf("Available colours:\n");
    printf("  0: BLACK\n");
    printf("  1: BLUE\n");
    printf("  2: GREEN\n");
    printf("  3: CYAN\n");
    printf("  4: RED\n");
    printf("  5: MAGENTA\n");
    printf("  6: BROWN\n");
    printf("  7: LIGHT_GREY\n");
    printf("  8: DARK_GREY\n");
    printf("  9: LIGHT_BLUE\n");
    printf("  A: LIGHT_GREEN\n");
    printf("  B: LIGHT_CYAN\n");
    printf("  C: LIGHT_RED\n");
    printf("  D: LIGHT_MAGENTA\n");
    printf("  E: LIGHT_BROWN\n");
    printf("  F: WHITE\n");
    printf("Enter the colour: ");
    clear_kb_buffer();
    while (getc() != 10);
    char* buffer = get_kb_buffer();
    buffer[strlen(buffer) - 1] = '\0'; /* remove '\n' */
    uint32_t colour = (uint32_t)hex_string_to_int(buffer);
    printf("Colour: %x\n", colour);
    set_putchar_colour(colour);
    clear_kb_buffer();
}

static void halt()
{
    printf("Halting system...\n");
    __asm__ __volatile__("hlt");
    printf("System halted\n");
}

static void hhalt()
{
    printf("Halting system... Safe reboot now.\n");
    __asm__ __volatile__("cli; hlt");
    printf("System halted\n");
}

static void help()
{
    printf("Available commands:\n");
    
    for (int i = 0; i < sizeof(commands) / sizeof(command_t); i++)
    {
        if (commands[i].hidden)
            continue;

        printf("  %s: %s\n", commands[i].cmd, commands[i].desc);
    }
}

static void kdump_stack()
{
    kdump((void*)get_stack_pointer(), 256);
}

static void ks_kdump()
{
    printf("Known addresses:\n");
    printf("  VIDEO_MEMORY: %x\n", VIDEO_MEMORY);
    printf("  BANNER: %x\n", &BANNER);
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
