#include "ushell.h"
#include "../syscalls/stdlib.h"
#include "../../utils/utils.h"

#include "../../display/display.h"

#define MAX_TOKENS 15

#define MAX_COMMANDS 256
#define MAX_SECTIONS_COMMANDS 25
#define MAX_SECTIONS SECTION_T_MAX

typedef struct
{
    const char* name;
    u_section_t section;
    u_command_t commands[MAX_SECTIONS_COMMANDS];
} u_command_section_t;

static u_section_t current_section = FOO;

static u_command_section_t command_sections[U_SECTION_T_MAX];

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
        // buffer[0] = '\0'; // Ensure empty string on read error
    }
    return buffer;
}

void ushell(void)
{
    char* buffer;
    char* tokens[MAX_TOKENS];
    // char* p;
    int token_count;
    int len;
    int i;

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
            // write(1, "Token: ", 7);
            // write(1, buffer, strlen(buffer));
            tokens[token_count++] = &buffer[i];
            while (buffer[i] != ' ' && buffer[i] != '\0') i++;
            if (buffer[i] == ' ')
            {
                buffer[i] = '\0'; // Terminate token
                i++;
                while (buffer[i] == ' ') i++;
            }
        }

        if (token_count > 0)
        {
            // printf("Command: %s\n", tokens[0]);
            for (i = 0; i < token_count; i++)
            {
                write(1, tokens[i], strlen(tokens[i]));
                write(1, " ", 1);
            }
            write(1, "\n", 1);
                
        }
        else
        {
            write(1, "No command entered\n", 19);
        }
    }
}