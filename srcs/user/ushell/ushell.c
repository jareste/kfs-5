#include "ushell.h"
#include "../syscalls/stdlib.h"
#include "../../utils/utils.h"

char* readline(char* prompt)
{
    static char buffer[1024];

    if (prompt)
        write(1, prompt, strlen(prompt));
    else
        write(1, "$ ", 2);

    read(0, buffer, 1024);
    return buffer;
}

void ushell(void)
{
    while (1)
    {
        char* buffer;
        buffer = readline(NULL);
    }
}