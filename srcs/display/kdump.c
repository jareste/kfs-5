#include "display.h"

static void kput_hex(uint8_t byte)
{
    const char hex_digits[] = "0123456789ABCDEF";
    putc(hex_digits[byte >> 4]);
    putc(hex_digits[byte & 0x0F]);
}

void kdump(void* addr, uint32_t size)
{
    if (addr == NULL)
    {
        printf("Invalid address: NULL\n");
        return;
    }

    // if ((uintptr_t)addr % 4 != 0)
    // {
    //     printf("Invalid address: Unaligned\n");
    //     return;
    // }

    uint8_t* p = (uint8_t*)addr;
    uint32_t i = 0;
    while (i <= size)
    {
        if (i % 16 == 0)
        {
            if (i != 0)
            {
                puts("  |");
                for (uint32_t j = i - 16; j < i; j++)
                {
                    uint8_t c = p[j];
                    if (c >= 32 && c <= 126)
                        putc(c);
                    else
                        putc('.');
                }
                puts("|");
            }
            puts("\n");
            if (i < size)
            {
                kput_hex((uint32_t)(p + i));
                puts(": ");
            }
        }

        if (i < size)
        {
            kput_hex(p[i]);
            puts(" ");
        }

        i++;
    }

    uint32_t remainder = size % 16;
    // printf("size: %d\n", size);
    // printf("remainder: %d\n", remainder);
    if (remainder != 0)
    {
        for (uint32_t j = 0; j < (16 - remainder); j++)
        {
            puts("   ");
        }
    }
    
    if (remainder > 0)
        puts("  |");

    for (uint32_t j = size - remainder; j < size; j++)
    {
        uint8_t c = p[j];
        if (c >= 32 && c <= 126)
            putc(c);
        else
            putc('.');
    }
    
    if (remainder > 0)
    {
        puts("|");
        puts("\n");
    }
}
