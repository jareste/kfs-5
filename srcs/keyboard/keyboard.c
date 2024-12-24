#include "../utils/stdint.h"
#include "../display/display.h"
#include "idt.h"

struct idt_entry idt[IDT_SIZE];
struct idt_ptr idtp;

void idt_set_gate(int idx, uint32_t base)
{
    unsigned short cs;
    __asm__ ("mov %%cs, %0" : "=r" (cs));

    idt[idx].base_low = LOW_16(base);
    idt[idx].base_high = HIGH_16(base);
    idt[idx].selector = cs;
    idt[idx].zero = 0;
    idt[idx].type_attr = 0x8E;
}


#define KEYBOARD_DATA_PORT 0x60
#define PIC1_COMMAND 0x20
#define PIC_EOI 0x20
/*
 * That will not work in 300 universes, but copilot said it
*/
unsigned char scancode_to_ascii[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', /* Backspace */
    '\t', /* Tab */
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', /* Enter */
    0, /* Control */
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, /* Left Shift */
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, /* Right Shift */
    '*', 0, /* Alt */
    ' ', /* Space */
    0 /* Caps Lock, Function keys, etc. */
};

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

void create_idt_entry(uint8_t num, uint32_t base)
{
    idt[num].base_low = base & 0xFFFF;
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].selector = 0x08;
    idt[num].zero = 0;
    idt[num].type_attr = 0x8E;
}


void keyboard_handler()
{
    puts("Keyboard interrupt handler entered\n");
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);
    puts("Scancode read\n");

    if (scancode < 128)
    {
        char key = scancode_to_ascii[scancode];
        if (key)
        {
            putc(key);
        }
    }

    outb(PIC1_COMMAND, PIC_EOI);
    puts("Interrupt acknowledged\n");
}



void default_interrupt_handler()
{
    puts("Unhandled interrupt\n");
    while (1);
}

void timer_handler()
{
    outb(0x20, 0x20);
}


void idt_init()
{
    idtp.limit = (sizeof(struct idt_entry) * IDT_SIZE) - 1;
    idtp.base = (uint32_t)&idt;

    for (int i = 0; i < IDT_SIZE; i++)
    {
        idt_set_gate(i, (uint32_t)default_interrupt_handler);
    }

    idt_set_gate(32, (uint32_t)timer_handler);

    idt_set_gate(33, (uint32_t)keyboard_handler);

    puts("IDT initialized\n");

    idt_load();
}






void init_interrupts()
{
    idt_init();


}
