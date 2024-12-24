#include "display.h"

int printf(const char *format, ...)
{
    const char *traverse;
    int i;
    char *s;

    void *arg = (void *)(&format + 1);

    for (traverse = format; *traverse != '\0'; traverse++)
    {
        while (*traverse != '%')
        {
            putc(*traverse);
            traverse++;
        }

        traverse++;

        switch (*traverse)
        {
        case 'c':
            i = *((int *)arg);
            putc(i);
            arg += sizeof(int);
            break;

        case 'd':
            /* TODO */
            break;
            // i = *((int *)arg);
            // if (i < 0)
            // {
            //     i = -i;
            //     putc('-');
            // }
            // // puts(itoa(i, 10));
            // arg += sizeof(int);
            // break;

        case 'x':
            /* TODO */
            break;
            // i = *((int *)arg);
            // puts("0x");
            // puts(itoa(i, 16));
            // arg += sizeof(int);
            // break;

        case 's':
            /* TODO */
            break;
            // s = *((char **)arg);
            // puts(s);
            // arg += sizeof(char *);
            // break;
        }
    }

    return i;
}