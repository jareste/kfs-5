#include "display/display.h"

void kernel_main()
{
    // char *video_memory = (char *)0xb8000;
    const char *message = "Hello, World!  ";
    // int i = 0;
    // int j = 0;
    // puts(message);
    puts_color(message, LIGHT_BLUE);

    // while (message[i] != '\0')
    // {
    //     video_memory[i * 2] = message[i];
    //     video_memory[i * 2 + 1] = 0x07; /* Attribute byte: light grey on black */
    //     i++;
    // }

    // int start_position = i;

    const char *message2 = "42Barcelona";
    // puts(message2);
    puts_color(message2, LIGHT_RED);
    // while (message2[j] != '\0')
    // {
    //     video_memory[(start_position + j) * 2] = message2[j];
    //     video_memory[(start_position + j) * 2 + 1] = 0x07; /* Attribute byte */
    //     j++;
    // }

    while (1);
}


