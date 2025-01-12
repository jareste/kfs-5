#include "../keyboard/idt.h"
#include "../utils/stdint.h"
#include "../io/io.h" // Include your I/O port functions (outb, inb)

#define PIT_CONTROL_PORT 0x43
#define PIT_CHANNEL0_PORT 0x40
#define PIT_BASE_FREQUENCY 1193182
#define PIT_FREQUENCY 100
#define PIC1_COMMAND 0x20
#define PIC_EOI 0x20

#define SECONDS_TO_TICKS(x) (x * PIT_FREQUENCY)

static uint32_t tick_count = 0;
static uint64_t seconds = 0;

void sleep(uint32_t seconds)
{
    uint32_t target_ticks = SECONDS_TO_TICKS(seconds);
    tick_count = 0;
    while (tick_count < target_ticks)
    {
        __asm__ __volatile__("hlt");  // Halt until next interrupt
    }
}

void irq_handler_timer()
{
    tick_count++;
    if (tick_count % PIT_FREQUENCY == 0)
    {
        seconds++;
    }
    /* I think i'm sending it somewhere else */
    outb(0x20, 0x20);
}

void get_uptime(uint64_t *uptime)
{
    *uptime = seconds;
}

void init_pit(uint32_t frequency)
{
    if (frequency < 18)
    {
        frequency = 18;
    }
    else if (frequency > PIT_BASE_FREQUENCY)
    {
        frequency = PIT_BASE_FREQUENCY;
    }

    /* CARE uint16!!! */
    uint16_t divisor = PIT_BASE_FREQUENCY / (frequency - 1);

    // printf("PIT divisor: %d\n", divisor);
    outb(PIT_CONTROL_PORT, 0x36);

    outb(PIT_CHANNEL0_PORT, (uint8_t)(divisor & 0xFF));  // Low byte
    outb(PIT_CHANNEL0_PORT, (uint8_t)((divisor >> 8) & 0xFF));  // High byte
}

void init_timer()
{
    disable_interrupts();
    init_pit(PIT_FREQUENCY);
    enable_interrupts();
}
