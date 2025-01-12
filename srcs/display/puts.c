#include "display.h"

int puts(const char *str)
{
    int i = 0;
    while (str[i])
    {
        putc(str[i]);
        i++;
    }
    return i;
}

int puts_color(const char *str, uint8_t color)
{
    int i = 0;
    while (str[i])
    {
        putc_color(str[i], color);
        i++;
    }
    return i;
}
