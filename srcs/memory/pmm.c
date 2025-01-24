#include "../utils/stdint.h"
#include "../utils/utils.h"
#include "../display/display.h"

#define PAGE_SIZE   0x1000
#define MAX_FRAMES 1024 * 1024  // Increase to support larger memory allocations

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
    for (uint32_t f = 0; f < (0x100000 / PAGE_SIZE); f++)
    {
        set_frame_used(f);
    }

    /* Initialize the rest as free
    */
    for (uint32_t f = (0x100000 / PAGE_SIZE); f < (MB(64) / PAGE_SIZE); f++)
    {
        set_frame_free(f);
    }

    /* TODO:
     * Maybe I must parse BIOS memory map to free all the regions that are free
     * and mark the used regions as used. So for now it is ok but not correct.
     */
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
