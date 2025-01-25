#include "../display/display.h"
#include "../keyboard/keyboard.h"
#include "../keyboard/signals.h"
#include "../utils/utils.h"
#include "../timers/timers.h"
#include "kshell.h"

#define MAX_COMMANDS 256
#define MAX_SECTIONS_COMMANDS 25
#define MAX_SECTIONS SECTION_T_MAX

typedef struct {
    const char* name;
    section_t section;
    command_t commands[MAX_SECTIONS_COMMANDS];
} command_section_t;

section_t current_section = GENERAL;

static void help();
static void ks_kdump();
static void reboot();
static void kdump_stack();
static void halt();
static void hhalt();
static void colour();
static void shutdown();
static void ksleep();
static void kuptime();
static void cmd_section();
static void help_global();
static void ks_kill();
static void trigger_interrupt_software_0();
static void trigger_interrupt_software_6();

static command_section_t command_sections[MAX_SECTIONS];

static command_t global_commands[] = {
    {"help", "Display this help message", help},
    {"?", NULL, help},
    {"h", NULL, help_global},
    {"reboot", "Reboot the system", reboot},
    {"clear", "Clear the screen", clear_screen},
    {"shutdown", "Shutdown the system", shutdown},
    {"sec", "Change the command section", cmd_section},
    {NULL, NULL, NULL}    
};

static command_t in_commands[] = {
    {"exit", "Exit the shell", NULL},
    {"color", "Set shell color", colour},
    {"uptime", "Get the system uptime in seconds.", kuptime},
    {"kill", "Kill a process", ks_kill},
    {NULL, NULL, NULL}
};

static command_t dcommand[] = {
    {"kdump", "Dump memory", ks_kdump},
    {"stack", "Dump stack", kdump_stack},
    {"halt", "Halt the system.", halt},
    {"sudo halt", "Halt the system. (Stops it).", hhalt},
    {"reg", "Dumps Registers", dump_registers},
    {"sleep", "Sleeps the kernel for 'n' seconds", ksleep},
    {"raise0", "Raise a division by zero exception", trigger_interrupt_software_0},
    {"raise6", "Raise an invalid opcode exception", trigger_interrupt_software_6},
    {NULL, NULL, NULL}
};

static void install_command(command_t* cmds, const char* cmd, const char* desc, void (*func)())
{
    for (int i = 0; i < MAX_SECTIONS_COMMANDS; i++)
    {
        if (cmds[i].cmd == NULL)
        {
            cmds[i].cmd = cmd;
            cmds[i].desc = desc;
            cmds[i].func = func;
            return;
        }
    }
}

void install_all_cmds(command_t* cmds, section_t section)
{
    for (int i = 0; i < MAX_SECTIONS_COMMANDS; i++)
    {
        if (cmds[i].cmd == NULL)
            break;
        
        install_command(command_sections[section].commands,\
         cmds[i].cmd, cmds[i].desc, cmds[i].func);
    }
}

static void init_section(const char* name, section_t section)
{
    command_sections[section].name = name;
    command_sections[section].section = section;
    memset(command_sections[section].commands, 0,\
    sizeof(command_sections[section].commands));
}

void init_kshell()
{
    init_section("global", GLOBAL);
    init_section("general", GENERAL);
    init_section("memory", MEMORY);
    init_section("misc", MISC);
    init_section("system", SYSTEM);
    init_section("debug", DEBUG);

    current_section = GLOBAL;

    install_all_cmds(global_commands, GLOBAL);
    install_all_cmds(in_commands, GENERAL);
    install_all_cmds(dcommand, DEBUG);
}

static void kuptime()
{
    uint64_t uptime;
    uptime = get_kuptime();
    printf("Uptime: %d seconds\n", uptime);
}

static void ksleep()
{
    printf("Enter the number of seconds to sleep: ");
    clear_kb_buffer();
    while (getc() != 10);
    char* buffer = get_kb_buffer();
    buffer[strlen(buffer) - 1] = '\0'; /* remove '\n' */
    uint32_t seconds = (uint32_t)hex_string_to_int(buffer);
    printf("Sleeping for %d seconds...\n", seconds);
    sleep(seconds);
    printf("Woke up!\n");
    clear_kb_buffer();
}

static void ks_kill()
{
    printf("Enter the PID to kill: ");
    clear_kb_buffer();
    while (getc() != 10);
    char* buffer = get_kb_buffer();
    buffer[strlen(buffer) - 1] = '\0'; /* remove '\n' */
    pid_t pid = (pid_t)hex_string_to_int(buffer);
    printf("Enter the signal to send: ");
    clear_kb_buffer();
    while (getc() != 10);
    buffer = get_kb_buffer();
    buffer[strlen(buffer) - 1] = '\0'; /* remove '\n' */
    int signal = (int)hex_string_to_int(buffer);
    printf("Killing PID: %d with signal: %d\n", pid, signal);
    kill(pid, signal);
    clear_kb_buffer();
}

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
    
    command_t* commands = command_sections[current_section].commands;

    for (int i = 0; i < MAX_SECTIONS_COMMANDS; i++)
    {
        if (commands[i].desc == NULL)
            continue;

        printf("  %s: %s\n", commands[i].cmd, commands[i].desc);
    }

}

static void help_global()
{
    printf("Available commands:\n");
    
    command_t* commands = command_sections[GLOBAL].commands;

    for (int i = 0; i < MAX_SECTIONS_COMMANDS; i++)
    {
        if (commands[i].desc == NULL)
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

static void shutdown()
{
    printf("Shutting down...\n");
    
    /* Delay for shutdown. */
    for (int i = 0; i < 50; i++) __asm__ __volatile__("hlt");

    /* QEMU-specific: sends shutdown signal to ACPI */
    outw(0x604, 0x2000);
}

static void cmd_section()
{
    printf("Available sections:\n");
    for (int i = 0; i < MAX_SECTIONS; i++)
    {
        printf("  %d: %s\n", i, command_sections[i].name);
    }
    printf("Enter the section: ");
    clear_kb_buffer();
    while (getc() != 10);
    char* buffer = get_kb_buffer();
    buffer[strlen(buffer) - 1] = '\0'; /* remove '\n' */
    uint32_t section = (uint32_t)hex_string_to_int(buffer);
    if (section < 0 || section >= MAX_SECTIONS)
    {
        printf("Invalid section\n");
        return;
    }
    printf("Section: %s\n", command_sections[section].name);
    current_section = section;
    clear_kb_buffer();
}

static bool check_global_cmd(char* cmd)
{
    command_t* commands = command_sections[GLOBAL].commands;
    for (int i = 0; i < MAX_SECTIONS_COMMANDS; i++)
    {
        if (commands[i].cmd && (strcmp(cmd, commands[i].cmd) == 0))
        {
            if (commands[i].func)
            {
                commands[i].func();
            }
            else
            {
                printf("Not implemented yet\n");
            }
            return true;
        }
    }
    return false;
}

static void trigger_interrupt_software_0()
{
    asm volatile (
        "mov $3, %eax\n"
        "xor %edx, %edx\n"
        "div %edx\n"
    );
}

static void trigger_interrupt_software_6()
{    
    int j = 3;
    int i = 0;
    printf("Kernel end: %d\n", j / i);
}

#include "../syscalls/syscalls.h"

void test_syscall() {
    // const char* msg = "Hello, world!\n";
    // size_t msg_len = 14; // Length of the message

    // asm volatile (
    //     "mov $1, %%eax\n"  // Syscall number (SYS_WRITE)
    //     "mov $1, %%ebx\n"  // File descriptor (stdout)
    //     "mov %0, %%ecx\n"  // Buffer to write (message)
    //     "mov %1, %%edx\n"  // Number of bytes to write (message length)
    //     "int $0x80\n"      // Trigger syscall
    //     :
    //     : "r"(msg), "r"(msg_len)
    //     : "eax", "ebx", "ecx", "edx"
    // );
    asm volatile (
        "mov $0, %%eax\n"  // Syscall number (SYS_EXIT)
        "mov $42, %%ebx\n" // Exit status
        "int $0x80\n"      // Trigger syscall
        :
        :
        : "eax", "ebx"
    );

}

// void kernel_main() {
// }

void kshell()
{
    int i = 0;
    command_t* commands;

    // init_syscalls();
    test_syscall();

    printf("jareste-OS> ");
    while (1)
    {
        char c = getc();
        if (c == 10)
        {
            char* buffer = get_kb_buffer();
            buffer[strlen(buffer) - 1] = '\0'; /* remove '\n' */

            if (check_global_cmd(buffer))
            {
                clear_kb_buffer();
                printf("jareste-OS> ");
                continue;
            }

            commands = command_sections[current_section].commands;
            for (i = 0; i < MAX_SECTIONS_COMMANDS; i++)
            {
                if (commands[i].cmd && (strcmp(buffer, commands[i].cmd) == 0))
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
            if (i == MAX_SECTIONS_COMMANDS)
            {
                printf("Command '%s' not found. Type 'help' for a list of available commands\n",\
                 buffer, command_sections[current_section].name, current_section);
            }

            clear_kb_buffer();
            printf("jareste-OS> ");
        }
    }
}
