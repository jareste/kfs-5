#include "../utils/stdint.h"

void outw(uint16_t port, uint16_t data)
{
    __asm__ __volatile__("outw %0, %1" : : "a"(data), "Nd"(port));
}

void outb(uint16_t port, uint8_t data)
{
	__asm__("out %%al, %%dx" : :"a"(data), "d"(port));
}

uint8_t inb(uint16_t port)
{
   uint8_t ret;
   asm volatile ("inb %%dx,%%al":"=a" (ret):"d" (port));
   return ret;
}

uint16_t inw(uint16_t port)
{
    uint16_t ret;
    __asm__ __volatile__("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}
