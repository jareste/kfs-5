#include "../utils/stdint.h"
#include "../display/display.h"

#define PAGE_SIZE 4096
#define PAGE_TABLE_ENTRIES 1024
#define PAGE_DIRECTORY_ENTRIES 1024

typedef uint32_t page_directory_t[PAGE_DIRECTORY_ENTRIES] __attribute__((aligned(PAGE_SIZE)));
typedef uint32_t page_table_t[PAGE_TABLE_ENTRIES] __attribute__((aligned(PAGE_SIZE)));

static page_directory_t page_directory __attribute__((aligned(PAGE_SIZE)));
static page_table_t page_table __attribute__((aligned(PAGE_SIZE)));

void paging_init()
{
    for (int i = 0; i < PAGE_DIRECTORY_ENTRIES; i++)
    {
        /* Set no present */
        page_directory[i] = 0x00000002;
    }

    for (int i = 0; i < PAGE_TABLE_ENTRIES; i++)
    {
        /* Present, read/write */
        page_table[i] = (i * PAGE_SIZE) | 0x03; 
    }

    /* Set page table into page dir */
    page_directory[0] = ((uint32_t)page_table) | 0x03; /* Same, present, read/write */

    /* Load it into cr3 */
    __asm__ __volatile__("mov %0, %%cr3" : : "r"(page_directory));

    /* Enable paging in cr0 */
    uint32_t cr0;
    __asm__ __volatile__("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000; /* Set the paging bit in cr0 */
    __asm__ __volatile__("mov %0, %%cr0" : : "r"(cr0));
}
