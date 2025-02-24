#include "stdint.h"
void uitoa(uint32_t num, char *buf)
{
    char temp[12];
    int i = 0;
    if (num == 0)
    {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    while (num > 0)
    {
        temp[i++] = '0' + (num % 10);
        num /= 10;
    }
    int j = 0;
    while (i > 0)
    {
        buf[j++] = temp[--i];
    }
    buf[j] = '\0';
}