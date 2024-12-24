#ifndef DISPLAY_H
#define DISPLAY_H

#include "../utils/utils.h"

#define VIDEO_MEMORY 0xb8000
#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25

#define BLACK           0x00
#define BLUE            0x01
#define GREEN           0x02
#define CYAN            0x03
#define RED             0x04
#define MAGENTA         0x05
#define BROWN           0x06
#define LIGHT_GREY      0x07
#define DARK_GREY       0x08
#define LIGHT_BLUE      0x09
#define LIGHT_GREEN     0x0A
#define LIGHT_CYAN      0x0B
#define LIGHT_RED       0x0C
#define LIGHT_MAGENTA   0x0D
#define LIGHT_BROWN     0x0E
#define WHITE           0x0F

#define putc(c) putc_color(c, LIGHT_GREY)
void putc_color(char c, uint8_t color);

#define puts(str) puts_color(str, LIGHT_GREY)
int puts_color(const char *str, uint8_t color);

int printf(const char *format, ...);

void outb(uint16_t port, uint8_t data);
uint8_t inb(uint16_t port);

void clear_screen();


#endif
