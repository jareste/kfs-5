#include "../utils/stdint.h"
#include "../utils/utils.h"
#include "../display/display.h"

#define PAGE_SIZE   0x1000
#define MAX_FRAMES  1024*256  // Example: up to 1 GB / 4 KB = 262,144 frames

static uint8_t frame_bitmap[MAX_FRAMES / 8]; 
// 1 bit per frame, so each bit says: used(1)/free(0). 
// For 256K frames, that’s 32 KB of bitmap.

static inline void set_frame_used(uint32_t frame_number) {
    frame_bitmap[frame_number / 8] |=  (1 << (frame_number % 8));
}
static inline void set_frame_free(uint32_t frame_number) {
    frame_bitmap[frame_number / 8] &= ~(1 << (frame_number % 8));
}
static inline bool is_frame_used(uint32_t frame_number) {
    return (frame_bitmap[frame_number / 8] & (1 << (frame_number % 8))) != 0;
}

/**
 * Example PMM init:
 *   - All frames default to used
 *   - Mark only a certain region as free, or parse BIOS map to free them properly
 */
void pmm_init()
{
    /* Mark everything used by default */
    // In pmm_init:
    // Mark frames from 0..1MB as used:
    for (uint32_t f = 0; f < (0x100000 / PAGE_SIZE); f++)
    {
        set_frame_used(f);
    }

    // Then mark frames from 1MB..64MB as free
    for (uint32_t f = (0x100000 / PAGE_SIZE); f < (64 * 1024 * 1024 / PAGE_SIZE); f++)
    {
        set_frame_free(f);
    }

    // Next, re‐mark the exact region the kernel occupies as used again
    // e.g. from 1MB to 1.2MB if that's your kernel size
    for (uint32_t f = 0x100000 / PAGE_SIZE; f < 0x120000 / PAGE_SIZE; f++)
    {
        set_frame_used(f);
    }
}

/**
 * allocate_frame:
 *   Finds the first free bit in the bitmap, sets it to used,
 *   returns physical address of that frame * PAGE_SIZE.
 */
uint32_t allocate_frame()
{
    for (uint32_t frame_number = 0; frame_number < MAX_FRAMES; frame_number++)
    {
        if (!is_frame_used(frame_number))
        {
            set_frame_used(frame_number);
            return frame_number * PAGE_SIZE;  // physical addr
        }
    }

    puts_color("vmalloc: out of memory!\n", RED);
    /* Out of memory! */
    return 0;
}

void free_frame(uint32_t phys_addr)
{
    uint32_t frame_number = phys_addr / PAGE_SIZE;
    set_frame_free(frame_number);
}
