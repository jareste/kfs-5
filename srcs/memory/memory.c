#include "memory.h"
#include "../utils/utils.h"
page_directory_t *page_directory;
page_table_t *first_page_table;

void init_paging()
{
    page_directory = (page_directory_t*)0x1000; // Address of the page directory
    first_page_table = (page_table_t*)0x2000;  // Address of the first page table

    for (int i = 0; i < PAGE_DIRECTORY_ENTRIES; i++)
    {
        page_directory->entries[i] = 0;
    }

    // Clear all entries in the first page table
    for (int i = 0; i < PAGE_TABLE_ENTRIES; i++)
    {
        first_page_table->entries[i] = (i * PAGE_SIZE) | PAGE_PRESENT | PAGE_WRITE;
    }

    // Map the first page table into the page directory
    page_directory->entries[0] = ((uint32_t)first_page_table) | PAGE_PRESENT | PAGE_WRITE;

    // Load the page directory into the CR3 register
    __asm__ __volatile__("mov %0, %%cr3" : : "r"(page_directory));

    // Enable paging by setting the PG bit in CR0
    uint32_t cr0;
    __asm__ __volatile__("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;  // Set the paging bit (PG)
    __asm__ __volatile__("mov %0, %%cr0" : : "r"(cr0));
}

// page_table_t* allocate_frame()
// {
//     page_table_t *page_table = (page_table_t*)kmalloc(PAGE_SIZE);
//     memset(page_table, 0, PAGE_SIZE);
//     return page_table;
// }

// void map_page(uint32_t virtual_address, uint32_t physical_address, uint32_t flags)
// {
//     uint32_t directory_index = virtual_address >> 22;
//     uint32_t table_index = (virtual_address >> 12) & 0x03FF;

//     page_table_t *page_table;
//     if (!(page_directory->entries[directory_index] & PAGE_PRESENT))
//     {
//         // Allocate a new page table
//         page_table = (page_table_t*)allocate_frame();
//         memset(page_table, 0, sizeof(page_table_t));
//         page_directory->entries[directory_index] = ((uint32_t)page_table) | PAGE_PRESENT | PAGE_WRITE;
//     }
//     else
//     {
//         // Get the existing page table
//         page_table = (page_table_t*)(page_directory->entries[directory_index] & ~0xFFF);
//     }

//     page_table->entries[table_index] = (physical_address & ~0xFFF) | flags;
// }

// #include "../display/display.h"

// #define KERNEL_HEAP_START 0x1000000  // 16 MB, starting address of the kernel heap
// #define KERNEL_HEAP_END   0x2000000  // 32 MB, end address of the kernel heap

// static uint32_t heap_top = KERNEL_HEAP_START; // Tracks the top of the heap

// /**
//  * Align a given address to a 4 KB boundary.
//  */
// static uint32_t align_to_page(uint32_t addr)
// {
//     return (addr + 0xFFF) & ~0xFFF;
// }

// /**
//  * Allocate a block of memory from the kernel heap.
//  *
//  * @param size The size of the memory block to allocate, in bytes.
//  * @return A pointer to the allocated memory, or NULL if allocation fails.
//  */
// void* kmalloc(uint32_t size)
// {
//     // Align the requested size to a 4 KB boundary
//     size = align_to_page(size);

//     // Check if there is enough space in the heap
//     if (heap_top + size > KERNEL_HEAP_END)
//     {
//         puts("kmalloc: Out of memory!\n");
//         return NULL;
//     }

//     // Allocate memory
//     void* allocated_memory = (void*)heap_top;
//     heap_top += size;

//     // Zero out the allocated memory (optional, for safety)
//     for (uint32_t i = 0; i < size; i++)
//     {
//         ((uint8_t*)allocated_memory)[i] = 0;
//     }

//     return allocated_memory;
// }

// /**
//  * Free a previously allocated block of memory (stub).
//  * Note: This implementation is a bump allocator and does not support freeing memory.
//  *
//  * @param addr The address of the memory block to free.
//  */
// void kfree(void* addr) {
//     // This is a stub function; freeing is not supported in a simple bump allocator.
//     (void)addr; // Suppress unused variable warning
//     puts("kfree: Memory freeing is not implemented!\n");
// }

// /**
//  * Initialize the kernel heap.
//  * This function can be extended for more complex heap setups.
//  */
// void init_heap() {
//     heap_top = KERNEL_HEAP_START;
//     puts("Kernel heap initialized.\n");
// }

