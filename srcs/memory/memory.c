#include "../utils/stdint.h"
#include "../display/display.h"
#include "../kshell/kshell.h"
#include "memory.h"
#include "mem_utils.h"
#include "../utils/utils.h"
#include "pmm.h"

extern uint32_t endkernel;
#define HEAP_START ((uintptr_t)&endkernel + 0x4000) // Start heap 4 KB after kernel

// #define HEAP_START  0x100000  /* Start of the heap */
// #define HEAP_START ((uintptr_t)&endkernel)
#define HEAP_SIZE_  0x100000  /* 1 MB heap size */
#define ALIGN(x) (((x) + 0xFFF) & ~0xFFF) /* 4 KB alignment */

typedef struct block_header {
    size_t size;               /* Size of the block (excluding header) */
    struct block_header* next; /* Pointer to the next free block */
    int free;                  /* Is this block free? (1 for yes, 0 for no) */
} block_header_t;

static block_header_t* free_list = NULL;
static void* heap_end;
static size_t HEAP_SIZE;

static void test_mem();
static void test_dynamic_heap_growth();
static void dump_page_table();
void dump_page_directory();


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

// void paging_init()
// {
//     for (int i = 0; i < PAGE_DIRECTORY_ENTRIES; i++)
//     {
//         /* Set no present */
//         page_directory[i] = 0x00000002;
//     }

//     for (int i = 0; i < PAGE_TABLE_ENTRIES; i++)
//     {
//         /* Present, read/write */
//         page_table[i] = (i * PAGE_SIZE) | 0x03; 
//     }

//     /* Set page table into page dir */
//     page_directory[0] = ((uint32_t)page_table) | 0x03; /* Same, present, read/write */

//     /* Load it into cr3 */
//     __asm__ __volatile__("mov %0, %%cr3" : : "r"(page_directory));

//     /* Enable paging in cr0 */
//     uint32_t cr0;
//     __asm__ __volatile__("mov %%cr0, %0" : "=r"(cr0));
//     cr0 |= 0x80000000; /* Set the paging bit in cr0 */
//     __asm__ __volatile__("mov %0, %%cr0" : : "r"(cr0));

//     HEAP_SIZE = HEAP_SIZE_;
//     /* Test page fault */
//     install_command("f pfw", "Force a page fault by writing to an unmapped address", m_force_page_fault_write);
//     install_command("f pfro", "Force a page fault by writing to a read-only page", m_force_page_fault_ro);
// }

void paging_init()
{
    memset(page_directory, 0, sizeof(page_directory));
    memset(page_table, 0, sizeof(page_table));

    // Map the first 4MB (first page table)
    for (int i = 0; i < PAGE_TABLE_ENTRIES; i++)
    {
        page_table[i] = (i * PAGE_SIZE) | 0x03; // Present, Read/Write
    }

    page_directory[0] = ((uint32_t)page_table) | 0x03; // Present, Read/Write

    // Load page directory
    __asm__ __volatile__("mov %0, %%cr3" : : "r"(page_directory));

    // Enable paging
    uint32_t cr0;
    __asm__ __volatile__("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000; // Enable paging
    __asm__ __volatile__("mov %0, %%cr0" : : "r"(cr0));

    install_command("f pfw", "Force a page fault by writing to an unmapped address", m_force_page_fault_write);
    install_command("f pfro", "Force a page fault by writing to a read-only page", m_force_page_fault_ro);
}



#define MAX_HEAP_SIZE (27 * 1024 * 1024) // 100 MB heap

void* kbrk(void* addr)
{
    if ((uintptr_t)addr > HEAP_START + MAX_HEAP_SIZE)
    {
        puts_color("Heap exceeds maximum limit!\n", RED);
        return (void*)-1;
    }

    uint32_t new_heap_end = ALIGN((uintptr_t)addr);
    uint32_t current_heap_end = ALIGN((uintptr_t)heap_end);

    while (current_heap_end < new_heap_end) {
        uint32_t page_directory_index = current_heap_end / (PAGE_SIZE * PAGE_TABLE_ENTRIES);
        uint32_t page_table_index = (current_heap_end % (PAGE_SIZE * PAGE_TABLE_ENTRIES)) / PAGE_SIZE;

        if (!(page_directory[page_directory_index] & 0x1)) {
            printf("Page directory %d not present, creating...\n", page_directory_index);
            // Allocate a page table, ensure alignment
            // page_table_t* new_page_table = (page_table_t*)ALIGN((uintptr_t)kmalloc(sizeof(page_table_t)));
            // if (!new_page_table) {
            //     puts_color("Failed to allocate new page table\n", RED);
            //     return (void*)-1;
            // }

            // page_table_t* new_page_table = (page_table_t*)ALIGN((uintptr_t)kmalloc(sizeof(page_table_t)));
            // if (!new_page_table || ((uintptr_t)new_page_table % PAGE_SIZE != 0)) {
            //     printf("Misaligned or failed allocation for page table: %p\n", new_page_table);
            //     return (void*)-1;
            // }

            // memset(new_page_table, 0, sizeof(page_table_t));
            // page_directory[page_directory_index] = ((uint32_t)new_page_table) | 0x03; // Mark present + RW


            uint32_t pt_phys = allocate_frame();
            if (!pt_phys) { /* out of memory error */ }
            memset((void*)pt_phys, 0, PAGE_SIZE); // if identity mapped

            page_directory[page_directory_index] = pt_phys | (PAGE_PRESENT | PAGE_RW);


        }

        // Map the page in the page table
        page_table_t* table = (page_table_t*)(page_directory[page_directory_index] & ~0xFFF);
        if (!((*table)[page_table_index] & 0x1)) {
            (*table)[page_table_index] = current_heap_end | 0x03; // Present + RW
        }

        current_heap_end += PAGE_SIZE;
    }

    heap_end = addr;
    return heap_end;

}



// void* kbrk(void* addr)
// {
//     if ((uintptr_t)addr > HEAP_START + HEAP_SIZE)
//     {
//         puts("Heap overflow!\n");
//         return (void*)-1;
//     }

//     heap_end = addr;
//     return heap_end;
// }


void* kmalloc(size_t size)
{
    size = ALIGN(size); /* Ensure size is aligned to 8 bytes */
    block_header_t* current = free_list;
    block_header_t* prev = NULL;

    printf("Searching for a block of size %z\n", size);

    /* Search for a suitable free block */
    while (current)
    {
        printf("Current block: %p, size: %z, free: %d\n", current, current->size, current->free);

        if (current->free && current->size >= size)
        {
            current->free = 0;

            printf("Found suitable block: %p\n", current);
            printf("Block size: %z, Requested size: %z\n", current->size, size);

            /* Split the block if it's too large */
            if (current->size > size + sizeof(block_header_t))
            {
                printf("Splitting block\n");
                block_header_t* new_block = (void*)((char*)current + sizeof(block_header_t) + size);

                /* Ensure the new block address is valid within the heap */
                if ((uintptr_t)new_block + sizeof(block_header_t) > (uintptr_t)heap_end)
                {
                    printf("New block address exceeds heap bounds, requesting more memory\n");

                    /* Request more memory for the heap */
                    if (kbrk((void*)((uintptr_t)new_block + sizeof(block_header_t))) == (void*)-1)
                    {
                        puts_color("WARNING: Out of memory during block split!\n", RED);
                        current->free = 1; // Restore the block as free
                        return NULL;
                    }
                }

                new_block->size = current->size - size - sizeof(block_header_t);
                new_block->next = current->next;
                new_block->free = 1;

                current->size = size;
                current->next = new_block;

                printf("New block created at %p with size %z\n", new_block, new_block->size);
            }

            return (void*)((char*)current + sizeof(block_header_t));
        }

        prev = current;
        current = current->next;
    }

    printf("No suitable block found for size %z, expanding heap\n", size);

    /* No suitable block found, expand the heap */
    block_header_t* new_block = (block_header_t*)heap_end;
    void* new_heap_end = (void*)((char*)heap_end + sizeof(block_header_t) + size);

    /* Request more heap space via kbrk */
    printf("Requesting more heap space until %p\n", new_heap_end);
    if (kbrk(new_heap_end) == (void*)-1)
    {
        puts_color("WARNING: Out of memory!\n", RED);
        return NULL;
    }

    // void* new_heap_end = (void*)((char*)heap_end + sizeof(block_header_t) + size);
    // if ((uintptr_t)new_heap_end > HEAP_START + MAX_HEAP_SIZE) {
    //     printf("Heap exceeds maximum size limit during kmalloc\n");
    //     return NULL;
    // }

    // if (kbrk(new_heap_end) == (void*)-1) {
    //     puts_color("Heap expansion failed in kmalloc\n", RED);
    //     return NULL;
    // }


    new_block->size = size;
    new_block->next = NULL;
    new_block->free = 0;

    if (prev)
    {
        prev->next = new_block;
    }
    else
    {
        free_list = new_block;
    }

    void* allocated_mem = (void*)((char*)new_block + sizeof(block_header_t));
    memset(allocated_mem, 0, size); // Clear the allocated memory
    return allocated_mem;
}


void kfree(void* ptr)
{
    if (!ptr) return;

    block_header_t* block = (block_header_t*)((char*)ptr - sizeof(block_header_t));
    block->free = 1;

    /* Coalesce adjacent free blocks */
    block_header_t* current = free_list;
    while (current)
    {
        if (current->free && current->next && current->next->free)
        {
            current->size += sizeof(block_header_t) + current->next->size;
            current->next = current->next->next;
        }
        current = current->next;
    }
}

size_t ksize(void* ptr)
{
    if (!ptr) return 0;

    block_header_t* block = (block_header_t*)((char*)ptr - sizeof(block_header_t));
    return block->size;
}

void heap_init()
{
    heap_end = (void*)ALIGN((uintptr_t)HEAP_START);
    free_list = (block_header_t*)heap_end;
    free_list->size = HEAP_SIZE_ - sizeof(block_header_t);
    free_list->next = NULL;
    free_list->free = 1;


    printf("Kernel ends at: %p\n", &endkernel);
    printf("Heap starts at: %p\n", HEAP_START);
    printf("Heap size: %z bytes\n", HEAP_SIZE_);

    install_command("dump pt", "Dump page table", dump_page_table);
    install_command("dump pd", "Dump page directory", dump_page_directory);
    install_command("tmem", "Test memory allocation", test_mem);
    install_command("theap", "Test dynamic heap growth", test_dynamic_heap_growth);
}

void dump_page_directory()
{
    printf("Page Directory:\n");
    for (int i = 0; i < PAGE_DIRECTORY_ENTRIES; i++)
    {
        if (page_directory[i] & 0x1) // Present
        {
            printf("Directory[%d]: %x\n", i, page_directory[i]);
            if (i == 1)
                printf("Directory[%d]: %x\n", 0, page_directory[0]);

            page_table_t* table = (page_table_t*)(page_directory[i] & ~0xFFF);
            for (int j = 0; j < PAGE_TABLE_ENTRIES; j++)
            {
                if ((*table)[j] & 0x1) // Present
                {
                    printf("Table[%d][%d]: %x\n", i, j, (*table)[j]);
                }
            }
        }
        else // Not present
        {
            if (i < 3)
                printf("Directory[%d]: %x Not present\n", i, page_directory[i]);
        }
    }

    page_table_t* table = (page_table_t*)(page_directory[0] & ~0xFFF);

    printf("Table[%d][%d]: %x\n", 0, 0, (*table)[0]);

}

void debug_page_mapping(uint32_t address);

static void dump_page_table(uint32_t directory_index)
{
    debug_page_mapping(0x00415000);
    // directory_index = 0;
    // page_table_t* table = (page_table_t*)(page_directory[directory_index] & ~0xFFF);
    // if (!table)
    // {
    //     printf("No page table for directory index %d\n", directory_index);
    //     return;
    // }

    // printf("Page Table[%d]:\n", directory_index);
    // for (int i = 0; i < PAGE_TABLE_ENTRIES; i++)
    // {
    //     if ((*table)[i] & 0x1) // Present
    //     {
    //         printf("Table[%d]: 0x%x\n", i, (*table)[i]);
    //     }
    // }
}


static void test_dynamic_heap_growth()
{
    printf("Initial heap_end: %p\n", heap_end);

    void* block1 = kmalloc(64);
    printf("Allocated block1: %p\n", block1);

    void* large_block = kmalloc(26 * 1024 * 1024); // Allocate 10 MB
    
    // void* large_block = kmalloc(0x200000); // Allocate 10 MB
    if (large_block)
    {
        printf("Allocated large_block: %p\n", large_block);
        put_hex(ksize(large_block));
        putc('\n');
        put_hex(2 * 1024 * 1024);
        putc('\n');
        for (int i = 0; i < 26 * 1024 * 1024; i++)
        {
            ((char*)large_block)[i] = 'A';
        }
        ((char*)large_block)[26 * 1024 * 1024 - 1] = '\0';
        puts_color(large_block + (26 * 1024 * 1024 -10), GREEN);
    }
    else
    {
        puts_color("Failed to allocate large block\n", RED);
    }

    printf("Heap_end: %p\n", heap_end);
    void* test = kmalloc(0x100000); // Allocate 4 KB

    printf("Allocated test: %p, size: 0x", test);
    put_hex(ksize(test));
    putc('\n');

    printf("New heap_end: %p\n", heap_end);

    kfree(block1);
    kfree(large_block);

    printf("Heap freed.\n");
}

static void test_mem()
{
    // void* block1 = (void*)0x10000C; // Test for SEGV. And it works! (no SEGV)
    void* block1 = kmalloc(64);
    printf("Allocated block1: %p\n", block1);
    printf("Size of block1: ");
    put_zu(ksize(block1));
    putc('\n');
    for (int i = 0; i < 64; i++)
    {
        ((char*)block1)[i] = 'A';
    }
    ((char*)block1)[63] = '\0';
    printf("Block1: %s\n", block1);
    kfree(block1);
    
    void* block2 = kmalloc(2 * 1024 * 1024);
    
    printf("Allocated block2: %p\n", block2);
    // while (1);

    printf("Size of block2: ");
    put_zu(ksize(block2));
    putc('\n');
    put_hex(ksize(block2));
    putc('\n');
    put_hex(2 * 1024 * 1024);
    putc('\n');

    kfree(block2);

    block2 = kmalloc(0x10000);
    printf("Reallocated block2: %p\n", block2);
    printf("Size of block2: ");
    put_zu(ksize(block2));
    putc('\n');

    kfree(block2);


    block2 = kmalloc(0x10);
    printf("Reallocated block2: %p\n", block2);
    printf("Size of block2: ");
    put_zu(ksize(block2));
    putc('\n');
    kfree(block2);


    printf("Memory freed\n");
}


void debug_page_mapping(uint32_t address)
{
    uint32_t page_directory_index = address / (PAGE_SIZE * PAGE_TABLE_ENTRIES);
    uint32_t page_table_index = (address % (PAGE_SIZE * PAGE_TABLE_ENTRIES)) / PAGE_SIZE;

    putc('\n');
    put_hex(address);
    putc('\n');
    printf("Address: %x\n", address);
    // printf("Page Directory Index: %d, Entry: %x\n", page_directory_index, page_directory[page_directory_index]);
    printf("Page Directory Index: %d, Page Table Index: %d, Entry: %x\n",\
     page_directory_index, page_table_index, page_directory[page_directory_index]);

    if (page_directory[page_directory_index] & 0x1) // Present
    {
        page_table_t* table = (page_table_t*)(page_directory[page_directory_index] & ~0xFFF);
        printf("Page Table Index: %d, Entry: %x\n", page_table_index, (*table)[page_table_index]);
    }
    else
    {
        printf("Page directory entry %d not present\n", page_directory_index);
    }
}
