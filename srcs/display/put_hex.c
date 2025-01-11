#include "display.h"

int put_hex(uint32_t n)
{
    char hex[16] = "0123456789ABCDEF";
    char buffer[9];
    int i = 0;

    for (int j = 0; j < 8; j++)
    {
        buffer[j] = hex[(n >> (28 - j * 4)) & 0xF];
    }
    buffer[8] = '\0';

    puts(buffer);

    return 0;
}