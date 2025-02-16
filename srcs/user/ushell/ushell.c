#include "ushell.h"
#include "../syscalls/stdlib.h"
#include "../../utils/utils.h"

#include "../../display/display.h"

#define MAX_TOKENS 15

#define MAX_COMMANDS 256
#define MAX_SECTIONS_COMMANDS 25
#define MAX_SECTIONS SECTION_T_MAX

void echo(char** args, int argc);
void u_exit(void);

typedef enum
{
    ECHO = 0,
    EXIT,
    BUILTIN_MAX
} builtin_def;

typedef struct
{
    const char* cmd;
    builtin_def def;
} builtin_t;

static builtin_t builtins[] = {
    {"echo", ECHO},
    {"exit", EXIT},
    {NULL, 0}
};

char* readline(char* prompt)
{
    static char buffer[1024];
    int bytes_read;


    if (prompt)
        write(1, prompt, strlen(prompt));
    else
        write(1, "$ ", 2);

    memset(buffer, 0, 1024);

    bytes_read = read(0, buffer, 1024);
    if (bytes_read > 0)
    {
        // buffer[bytes_read] = '\0';
    }
    else
    {
        write(1, "read() failed\n", 14);
        memset(buffer, 0, 1024);
    }
    return buffer;
}

void exec_builtin(char** args, int argc)
{
    int i;

    i = 0;
    while (builtins[i].cmd != NULL)
    {
        if (strcmp(args[0], builtins[i].cmd) == 0)
        {
            switch (builtins[i].def)
            {
                case ECHO:
                    echo(args, argc);
                    break;
                case EXIT:
                    u_exit();
                    break;
                default:
                    break;
            }
            return;
        }
        i++;
    }

    printf("Command '%s' not found\n", args[0]);
}

void ushell(void)
{
    char* buffer;
    char* tokens[MAX_TOKENS];
    // char* p;
    int token_count;
    int len;
    int i;

    printf("Welcome to ushell\n");

    while (1)
    {
        buffer = readline(NULL);

        len = strlen(buffer);
        if (len == 0)
            continue;

        token_count = 0;

        i = 0;
        while (i < len && token_count < MAX_TOKENS)
        {
            tokens[token_count++] = &buffer[i];
            while (buffer[i] != ' ' && buffer[i] != '\0') i++;
            if (buffer[i] == ' ')
            {
                buffer[i] = '\0';
                i++;
                while (buffer[i] == ' ') i++;
            }
        }

        exec_builtin(tokens, token_count);
    }
}

void echo(char** args, int argc)
{
    for (int i = 1; i < argc; i++)
    {
        write(1, args[i], strlen(args[i]));
        write(1, " ", 1);
    }
    write(1, "\n", 1);
}

void u_exit(void)
{
    // write(1, "Exiting ushell\n", 15);
    // printf("PID: %d\n", get_pid());
    // kill(get_pid(), 2);
    exit(0);
}
