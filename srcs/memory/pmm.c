#include <stdint.h>
#include <stdbool.h>

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
    for (uint32_t i = 0; i < MAX_FRAMES / 8; i++)
    {
        frame_bitmap[i] = 0xFF;
    }

    // For example, let's just assume the first 16 MB (minus kernel) is free
    // Real code: parse your memory map to set frames free or used properly
    const uint32_t frames_16MB = (16 * 1024 * 1024) / PAGE_SIZE;
    for (uint32_t f = 0; f < frames_16MB; f++)
    {
        set_frame_free(f);
    }

    // You must also mark frames where the kernel resides as used
    // e.g. if your kernel ends at physical ~1 MB, you re-mark those frames used
    // This depends on your actual link layout.
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

    /* Out of memory! */
    return 0;
}

void free_frame(uint32_t phys_addr)
{
    uint32_t frame_number = phys_addr / PAGE_SIZE;
    set_frame_free(frame_number);
}
