#include "../utils/stdint.h"
#include "../display/display.h"
#include "../kshell/kshell.h"

#define PAGE_SIZE 4096
#define PAGE_TABLE_ENTRIES 1024
#define PAGE_DIRECTORY_ENTRIES 1024

typedef uint32_t page_directory_t[PAGE_DIRECTORY_ENTRIES] __attribute__((aligned(PAGE_SIZE)));
typedef uint32_t page_table_t[PAGE_TABLE_ENTRIES] __attribute__((aligned(PAGE_SIZE)));

static page_directory_t page_directory __attribute__((aligned(PAGE_SIZE)));
static page_table_t page_table __attribute__((aligned(PAGE_SIZE)));

static void m_force_page_fault_write()
{
    uint32_t *invalid_addr = (uint32_t*)0xC0000000; // High address not mapped
    *invalid_addr = 0xDEADBEEF; // Attempt to write
}

static void m_force_page_fault_ro()
{
    page_table[1] &= ~0x2; // Mark page 0x1000 as read-only
    __asm__ __volatile__("invlpg (%0)" : : "r"(0x1000) : "memory"); // Invalidate TLB for 0x1000
    uint32_t *read_only_addr = (uint32_t*)0x1000;
    *read_only_addr = 0xDEADBEEF; // Attempt to write
}

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

    /* Test page fault */
    install_command("f pfw", "Force a page fault by writing to an unmapped address", m_force_page_fault_write);
    install_command("f pfro", "Force a page fault by writing to a read-only page", m_force_page_fault_ro);
}
