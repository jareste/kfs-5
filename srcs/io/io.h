#ifndef IO_H
#define IO_H

#include "../utils/stdint.h"

void outb(uint16_t port, uint8_t data);
uint8_t inb(uint16_t port);
void outw(uint16_t port, uint16_t data);

#endif