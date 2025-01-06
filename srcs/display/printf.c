#include "display.h"
#include "../utils/utils.h"

int printf(const char *format, ...)
{
    const char *traverse;
    int i;
    char *s;
    char buffer[32];

    void *arg = (void *)(&format + 1);

    for (traverse = format; *traverse != '\0'; traverse++)
    {
        while (*traverse != '%' && *traverse != '\0')
        {
            putc(*traverse);
            traverse++;
        }

        if (*traverse == '\0')
            break;

        traverse++;

        switch (*traverse)
        {
        case 'c':
            i = *(int *)arg;
            putc(i);
            arg = (void *)((int *)arg + 1);
            break;

        case 'd':
            i = *(int *)arg;
            itoa(i, buffer, 10);
            for (char *p = buffer; *p != '\0'; p++)
            {
                putc(*p);
            }
            arg = (void *)((int *)arg + 1);
            break;

        case 's':
            s = *(char **)arg;
            for (; *s != '\0'; s++)
            {
                putc(*s);
            }
            arg = (void *)((char **)arg + 1);
            break;

        case 'u':
            i = *(unsigned int *)arg;
            itoa(i, buffer, 10);
            for (char *p = buffer; *p != '\0'; p++)
            {
                putc(*p);
            }
            arg = (void *)((unsigned int *)arg + 1);
            break;

        default:
            putc('%');
            putc(*traverse);
            break;
        }
    }

    return i;
}
