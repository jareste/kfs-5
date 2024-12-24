void kernel_main()
{
    char *video_memory = (char *)0xb8000;
    const char *message = "Hello, World!";
    int i = 0;

    while (message[i] != '\0')
    {
        video_memory[i * 2] = message[i];
        video_memory[i * 2 + 1] = 0x07; /* Attribute byte. */
        i++;
    }

    while (1);
}

