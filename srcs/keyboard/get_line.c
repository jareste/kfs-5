#include "keyboard.h"
#include "../utils/utils.h"
#include "../tasks/task.h"
char* get_line()
{
    clear_kb_buffer();
    while (getc() != '\n') scheduler();
    char* buffer = get_kb_buffer();
    buffer[strlen(buffer) - 1] = '\0'; /* remove '\n' */
    return buffer;
}
