#ifndef KEYBOARD_H
#define KEYBOARD_H

char get_last_char();
char get_last_char_blocking();
#define getc() get_last_char()
char* get_kb_buffer();
void clear_kb_buffer();

#endif