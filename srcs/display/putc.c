#include "display.h"

static int cursor_position = 0;

static void scroll_screen()
{
    char *video_memory = (char *)VIDEO_MEMORY;

    for (int i = 0; i < (SCREEN_HEIGHT - 1) * SCREEN_WIDTH * 2; i++)
    {
        video_memory[i] = video_memory[i + SCREEN_WIDTH * 2];
    }

    for (int i = (SCREEN_HEIGHT - 1) * SCREEN_WIDTH * 2;\
            i < SCREEN_HEIGHT * SCREEN_WIDTH * 2; i += 2)
    {
        video_memory[i] = ' ';
        video_memory[i + 1] = LIGHT_GREY;
    }
}

void putc_color(char c, uint8_t color)
{
    char *video_memory = (char *)VIDEO_MEMORY;

    if (c == '\n')
    {
        cursor_position += SCREEN_WIDTH - (cursor_position % SCREEN_WIDTH);
    }
    else if (c == '\r')
    {
        cursor_position -= (cursor_position % SCREEN_WIDTH);
    }
    else
    {
        video_memory[cursor_position * 2] = c;
        video_memory[cursor_position * 2 + 1] = color;
        cursor_position++;
    }

    if (cursor_position >= SCREEN_WIDTH * SCREEN_HEIGHT)
    {
        scroll_screen();
        cursor_position -= SCREEN_WIDTH;
    }
}
