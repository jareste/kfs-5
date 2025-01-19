#include "display.h"
void put_zu(size_t value)
{
    char buffer[20];
    int i = 0;

    if (value == 0)
    {
        putc('0');
        return;
    }

    while (value > 0)
    {
        buffer[i++] = '0' + (value % 10);
        value /= 10;
    }

    while (i > 0)
    {
        putc(buffer[--i]);
    }
}