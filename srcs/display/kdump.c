#include "display.h"

void kdump(void* addr, uint32_t size)
{
    uint8_t* p = (uint8_t*)addr;
    uint32_t i = 0;
    while (i < size)
    {
        if (i % 16 == 0)
        {
            puts("\n");
            put_hex((uint32_t)(p + i));
            puts(": ");
        }
        put_hex(p[i]);
        puts(" ");
        i++;
    }
    puts("\n");

}