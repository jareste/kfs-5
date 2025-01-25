#include "../utils/stdint.h"
#include "../display/display.h"
#include "../io/io.h"
#include "../timers/timers.h"
#include "idt.h"
#include "../memory/memory.h"

#define KEYBOARD_DATA_PORT 0x60

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

static void set_kb_char(char c)
{
    keyboard_buffer[keyb_buff_end] = c;
    
    keyb_buff_end = (keyb_buff_end + 1) % KEYBOARD_BUFFER_SIZE;
}

static void delete_last_kb_char()
{
    if (keyb_buff_end == 0)
    {
        return;
    }
    else
    {
        keyb_buff_end--;
    }
    keyboard_buffer[keyb_buff_end] = 0;
}

void clear_kb_buffer()
{
    keyb_buff_start = 0;
    keyb_buff_end = 0;
    memset(keyboard_buffer, 0, KEYBOARD_BUFFER_SIZE);
}

void keyboard_handler()
{
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);
    char key = 0;

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
        // move_cursor_left();
    }
    else if (scancode == 0x4D) // Right Arrow
    {
        // move_cursor_right();
    }
    else if (scancode == 0x48)
    {
        // move_cursor_up();
    }
    else if (scancode == 0x50) // Down Arrow
    {
        // move_cursor_down();
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
                delete_last_kb_char();
            }
        }
        else 
        {
            key = shift_pressed ? shifted_scancode_to_ascii[scancode] : scancode_to_ascii[scancode];
            if (ctrl_pressed && ((key == 'r') || (key == 'R')))
            {
                clear_screen();
                key = 0;
            }
            if (key)
            {
                putc(key);
                set_kb_char(key);
            }
        }
    }

    outb(PIC1_COMMAND, PIC_EOI);
}
