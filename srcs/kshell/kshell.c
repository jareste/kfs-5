#include "../display/display.h"
#include "../keyboard/keyboard.h"

/* must implement getc() */

void kshell()
{
    printf("jareste-OS> ");
    while (1)
    {
        char c = getc();
        if (c == 10)
        {
            printf("-%d-", c);
            char* buffer = get_kb_buffer();

            printf("-%d-", c);
            if (strcmp(buffer, "exit") == 0)
            {
                printf("Exiting shell...\n");
                break;
            }
            else
            {
                printf("Unknown command: %s\n", buffer);
            }
            printf("jareste-OS> ");
            printf("\n|%d|\n", c);           
        }
    }
}