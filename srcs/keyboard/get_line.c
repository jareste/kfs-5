#include "keyboard.h"
#include "../utils/utils.h"
char* get_line()
{
    clear_kb_buffer();
    while (getc() != '\n');
    char* buffer = get_kb_buffer();
    buffer[strlen(buffer) - 1] = '\0'; /* remove '\n' */
    return buffer;
}
