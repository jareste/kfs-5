#include "../utils/stdint.h"
#include "../display/display.h"
#include "idt.h"

#define KEYBOARD_DATA_PORT 0x60
#define PIC1_COMMAND 0x20
#define PIC_EOI 0x20

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64

/* Scancode to ASCII mapping for US QWERTY layout */
char scancode_to_ascii[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', // Backspace
    '\t', // Tab
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', // Enter
    0,    // Left Control
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0,    // Left Shift
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, // Right Shift
    '*',
    0,    // Left Alt
    ' ',  // Spacebar
    0,    // Caps Lock
    0,    // F1
    0,    // F2
    0,    // F3
    0,    // F4
    0,    // F5
    0,    // F6
    0,    // F7
    0,    // F8
    0,    // F9
    0,    // F10
    0,    // Num Lock
    0,    // Scroll Lock
    0,    // Home
    0,    // Up Arrow
    0,    // Page Up
    '-',
    0,    // Left Arrow
    0,
    0,    // Right Arrow
    '+',
    0,    // End
    0,    // Down Arrow
    0,    // Page Down
    0,    // Insert
    0,    // Delete
    0, 0, 0, 0, 0, 0, 0, 0, // Unused
    0, 0, 0, 0, 0, 0, 0, 0, // Unused
};

/* Shifted scancode to ASCII mapping for US QWERTY layout */
char shifted_scancode_to_ascii[128] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b', // Backspace
    '\t', // Tab
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', // Enter
    0,    // Left Control
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0,    // Left Shift
    '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, // Right Shift
    '*',
    0,    // Left Alt
    ' ',  // Spacebar
    0,    // Caps Lock
    0,    // F1
    0,    // F2
    0,    // F3
    0,    // F4
    0,    // F5
    0,    // F6
    0,    // F7
    0,    // F8
    0,    // F9
    0,    // F10
    0,    // Num Lock
    0,    // Scroll Lock
    0,    // Home
    0,    // Up Arrow
    0,    // Page Up
    '_',
    0,    // Left Arrow
    0,
    0,    // Right Arrow
    '+',
    0,    // End
    0,    // Down Arrow
    0,    // Page Down
    0,    // Insert
    0,    // Delete
    0, 0, 0, 0, 0, 0, 0, 0, // Unused
    0, 0, 0, 0, 0, 0, 0, 0, // Unused
};

#define KEYBOARD_BUFFER_SIZE 256
static char keyboard_buffer[KEYBOARD_BUFFER_SIZE] = {0};
static int keyb_buff_start = 0;
static int keyb_buff_end = 0;

bool shift_pressed = false;
bool ctrl_pressed = false;

char get_last_char()
{
    if (keyb_buff_start == keyb_buff_end)
    {
        return '\0';
    }

    char c = keyboard_buffer[keyb_buff_start];
    keyb_buff_start = (keyb_buff_start + 1) % KEYBOARD_BUFFER_SIZE;
    printf("|%d|", c);
    return c;
}

char get_last_char_blocking()
{
    char c;
    while ((c = get_last_char()) == '\0');
    return c;
}

char* get_kb_buffer()
{
    return keyboard_buffer;
}

void keyboard_handler()
{
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);

    if (scancode == 0x2A || scancode == 0x36)
    {
        shift_pressed = true; // Left or Right Shift pressed
    }
    else if (scancode == 0x1D)
    {
        ctrl_pressed = true; // Control pressed
    }
    else if (scancode == 0xAA || scancode == 0xB6)
    {
        shift_pressed = false; // Left or Right Shift released
    }
    else if (scancode == 0x9D)
    {
        ctrl_pressed = false; // Control released
    }
    else if (scancode == 0x4B) // Left Arrow
    {
        move_cursor_left();
    }
    else if (scancode == 0x4D) // Right Arrow
    {
        move_cursor_right();
    }
    else if (scancode == 0x48)
    {
        move_cursor_up();
    }
    else if (scancode == 0x50) // Down Arrow
    {
        move_cursor_down();
    }
    else if (scancode == 0x0F) // Tab
    {
        puts("    ");
    }
    else if (scancode == 0x53) // Delete
    {
        delete_actual_char();
    }
    else if (scancode < 128)
    {

        if (scancode == 0x01)
        {
            clear_screen();
        }
        else if (scancode == 0x0E)
        {
            if (ctrl_pressed)
            {
                delete_until_char();
            }
            else
            {
                delete_last_char();
            }
        }
        else 
        {
            char key = shift_pressed ? shifted_scancode_to_ascii[scancode] : scancode_to_ascii[scancode];
            if (ctrl_pressed && ((key == 'r') || (key == 'R')))
            {
                clear_screen();
                key = 0;
            }
            if (key)
            {
                putc(key);
                printf("|%d|", key);
                keyboard_buffer[keyb_buff_end] = key;
                keyb_buff_end = (keyb_buff_end + 1) % KEYBOARD_BUFFER_SIZE;
            }
        }
    }

    outb(PIC1_COMMAND, PIC_EOI);
}





// void idt_init()
// {
//     idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
//     idtp.base = (uint32_t)&idt;

//     for (int i = 0; i < 256; i++)
//     {
//         // idt_set_gate(i, (uint32_t)default_interrupt_handler);
//     }

//     // idt_set_gate(32, (uint32_t)timer_handler);

//     idt_set_gate(33, (uint32_t)keyboard_handler);

//     puts("IDT initialized\n");

//     idt_load();
// }

void enable_interrupts(void)
{
	__asm__ __volatile__("sti");
}

// disable hardware interrupts
void disable_interrupts(void)
{
	__asm__ __volatile__("cli");
}

void isr_handler(registers reg, uint32_t intr_no, uint32_t err_code, error_state stack)
{
	(void) reg;
    (void) stack;
    (void) err_code;
    (void) intr_no;
    puts("Interrupt handler entered\n");
	// while (1)
	// {
		
	// }
}

void irq_handler(registers reg, uint32_t intr_no, uint32_t err_code, error_state stack)
{
	(void) err_code;
	
	// ack pic
    // puts("Interrupt handler entered\n");
    // printf("Interrupt number: %d\n", intr_no);
	disable_interrupts();
	outb(PIC_EOI, PIC1_COMMAND);
	if (intr_no == 1)
	{
		keyboard_handler();
	}
	// while (1)
	// {
		
	// }
	
	enable_interrupts();
}

void init_interrupts()
{
    idt_set_gate(0, (uint32_t)isr_handler_0);
    idt_set_gate(1, (uint32_t)isr_handler_1);
    idt_set_gate(2, (uint32_t)isr_handler_2);
    idt_set_gate(3, (uint32_t)isr_handler_3);
    idt_set_gate(4, (uint32_t)isr_handler_4);
    idt_set_gate(5, (uint32_t)isr_handler_5);
    idt_set_gate(6, (uint32_t)isr_handler_6);
    idt_set_gate(7, (uint32_t)isr_handler_7);
    idt_set_gate(8, (uint32_t)isr_handler_8);
    idt_set_gate(9, (uint32_t)isr_handler_9);
    idt_set_gate(10, (uint32_t)isr_handler_10);
    idt_set_gate(11, (uint32_t)isr_handler_11);
    idt_set_gate(12, (uint32_t)isr_handler_12);
    idt_set_gate(13, (uint32_t)isr_handler_13);
    idt_set_gate(14, (uint32_t)isr_handler_14);
    idt_set_gate(15, (uint32_t)isr_handler_15);
    idt_set_gate(16, (uint32_t)isr_handler_16);
    idt_set_gate(17, (uint32_t)isr_handler_17);
    idt_set_gate(18, (uint32_t)isr_handler_18);
    idt_set_gate(19, (uint32_t)isr_handler_19);
    idt_set_gate(20, (uint32_t)isr_handler_20);
    idt_set_gate(21, (uint32_t)isr_handler_21);
    idt_set_gate(22, (uint32_t)isr_handler_22);
    idt_set_gate(23, (uint32_t)isr_handler_23);
    idt_set_gate(24, (uint32_t)isr_handler_24);
    idt_set_gate(25, (uint32_t)isr_handler_25);
    idt_set_gate(26, (uint32_t)isr_handler_26);
    idt_set_gate(27, (uint32_t)isr_handler_27);
    idt_set_gate(28, (uint32_t)isr_handler_28);
    idt_set_gate(29, (uint32_t)isr_handler_29);
    idt_set_gate(30, (uint32_t)isr_handler_30);
    idt_set_gate(31, (uint32_t)isr_handler_31);

    /* remap PIC */

    unsigned char a1 = inb(0x21);
    unsigned char a2 = inb(0xA1);

    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);
    outb(0xA1, 0x28);
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    outb(0x21, a1);
    outb(0xA1, a2);

    idt_set_gate(32, (uint32_t)irq_handler_0);
    idt_set_gate(33, (uint32_t)irq_handler_1);
    idt_set_gate(34, (uint32_t)irq_handler_2);
    idt_set_gate(35, (uint32_t)irq_handler_3);
    idt_set_gate(36, (uint32_t)irq_handler_4);
    idt_set_gate(37, (uint32_t)irq_handler_5);
    idt_set_gate(38, (uint32_t)irq_handler_6);
    idt_set_gate(39, (uint32_t)irq_handler_7);
    idt_set_gate(40, (uint32_t)irq_handler_8);
    idt_set_gate(41, (uint32_t)irq_handler_9);
    idt_set_gate(42, (uint32_t)irq_handler_10);
    idt_set_gate(43, (uint32_t)irq_handler_11);
    idt_set_gate(44, (uint32_t)irq_handler_12);
    idt_set_gate(45, (uint32_t)irq_handler_13);
    idt_set_gate(46, (uint32_t)irq_handler_14);
    idt_set_gate(47, (uint32_t)irq_handler_15);


    register_idt();
}
