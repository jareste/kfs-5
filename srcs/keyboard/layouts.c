#include "../utils/utils.h"
#include "keyboard.h"

typedef struct
{
    char* scancode_to_ascii;
    char* shifted_scancode_to_ascii;
} layouts_t;

/* Scancode to ASCII mapping for US QWERTY layout */
char QWERTY_ENG_scancode_to_ascii[128] = {
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
char QWERTY_ENG_shifted_scancode_to_ascii[128] = {
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

char QWERTY_ES_scancode_to_ascii[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '\'', '\b', // Backspace
    '\t', // Tab
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '`', '+', '\n', // Enter
    0,    // Left Control
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', 'n', '{', '|',
    0,    // Left Shift
    '}', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '-', 0, // Right Shift
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

char QWERTY_ES_shifted_scancode_to_ascii[128] = {
    0,  27, '!', '"', '#', '$', '%', '&', '/', '(', ')', '=', '?', '\b', // Backspace
    '\t', // Tab
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '^', '*', '\n', // Enter
    0,    // Left Control
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', 'n', '[', '\'',
    0,    // Left Shift
    ']', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', ';', ':', '_', 0, // Right Shift
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

layouts_t layouts[MAX_KEYBOARD] = {
    {QWERTY_ENG_scancode_to_ascii, QWERTY_ENG_shifted_scancode_to_ascii},
    {QWERTY_ES_scancode_to_ascii, QWERTY_ES_shifted_scancode_to_ascii},
};

static uint8_t current_layout = QWERTY_ENG;

void set_keyboard_layout(uint8_t layout)
{
    current_layout = layout;
}

char get_ascii_char(uint8_t scancode, bool shifted)
{
    if (scancode & 0x80 || scancode >= 128)
        return 0;

    if (shifted)
        return layouts[current_layout].shifted_scancode_to_ascii[scancode];
    else
        return layouts[current_layout].scancode_to_ascii[scancode];
}
