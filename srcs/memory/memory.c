#include "../utils/stdint.h"
#include "../display/display.h"
#include "../kshell/kshell.h"
#include "memory.h"
#include "mem_utils.h"
#include "../utils/utils.h"

#define HEAP_START  0x100000  /* Start of the heap */
#define HEAP_SIZE_  0x100000  /* 1 MB heap size */
#define ALIGN(x) (((x) + 7) & ~7) /* 8-byte alignment */

typedef struct block_header {
    size_t size;               /* Size of the block (excluding header) */
    struct block_header* next; /* Pointer to the next free block */
    int free;                  /* Is this block free? (1 for yes, 0 for no) */
} block_header_t;

static block_header_t* free_list = NULL;
static void* heap_end = (void*)HEAP_START;
static size_t HEAP_SIZE;

static void test_mem();
static void test_dynamic_heap_growth();
static void dump_page_table();
static void dump_page_directory();


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


#define MAX_HEAP_SIZE (100 * 1024 * 1024) // 100 MB heap

void* kbrk(void* addr)
{
    if ((uintptr_t)addr > HEAP_START + MAX_HEAP_SIZE)
    {
        puts_color("Heap exceeds maximum limit!\n", RED);
        return (void*)-1;
    }

    uint32_t new_heap_end = ALIGN((uintptr_t)addr);
    uint32_t current_heap_end = ALIGN((uintptr_t)heap_end);

    while (current_heap_end < new_heap_end)
    {
        uint32_t page_directory_index = current_heap_end / (PAGE_SIZE * PAGE_TABLE_ENTRIES);
        uint32_t page_table_index = (current_heap_end % (PAGE_SIZE * PAGE_TABLE_ENTRIES)) / PAGE_SIZE;

        // Allocate a new page table if needed
        printf("Page Directory Index: %d\n", page_directory_index);
        if (!(page_directory[page_directory_index] & 0x1)) // Check if page table is present
        {
            page_table_t* new_page_table = (page_table_t*)kmalloc(sizeof(page_table_t)); // Allocate page table dynamically
            if (!new_page_table)
            {
                puts_color("Failed to allocate page table!\n", RED);
                return (void*)-1;
            }

            memset(new_page_table, 0, sizeof(page_table_t));
            page_directory[page_directory_index] = ((uint32_t)new_page_table) | 0x03; // Present, Read/Write
            __asm__ __volatile__("invlpg (%0)" : : "r"(new_page_table) : "memory");  // Invalidate TLB
        }

        // Map the current page in the page table
        page_table_t* page_table = (page_table_t*)(page_directory[page_directory_index] & ~0xFFF);
        (*page_table)[page_table_index] = current_heap_end | 0x03; // Present, Read/Write

        current_heap_end += PAGE_SIZE;
    }

    heap_end = addr; // Update the heap end
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

    /* Search for a suitable free block */
    printf("Searching for a block of size %z\n", size);
    while (current)
    {
        if (current->free && current->size >= size)
        {
            current->free = 0;

            /* Split the block if it's too large */
            if (current->size > size + sizeof(block_header_t))
            {
                block_header_t* new_block = (void*)((char*)current + sizeof(block_header_t) + size);
                new_block->size = current->size - size - sizeof(block_header_t);
                new_block->next = current->next;
                new_block->free = 1;

                current->size = size;
                current->next = new_block;
            }

            return (void*)((char*)current + sizeof(block_header_t));
        }

        prev = current;
        current = current->next;
    }

    printf("No suitable block found for size %z\n", size);
    /* No suitable block found, expand the heap */
    block_header_t* new_block = (block_header_t*)heap_end;
    void* new_heap_end = (void*)((char*)heap_end + sizeof(block_header_t) + size);

    /* Request more heap space via kbrk */
    if (kbrk(new_heap_end) == (void*)-1)
    {
        puts_color("WARNING: Out of memory!\n", RED);
        return NULL;
    }

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

    printf("Allocated new block: %p\n", new_block);
    return (void*)((char*)new_block + sizeof(block_header_t));
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
    free_list = (block_header_t*)HEAP_START;
    free_list->size = HEAP_SIZE - sizeof(block_header_t);
    free_list->next = NULL;
    free_list->free = 1;

    heap_end = (void*)HEAP_START;

    install_command("dump pt", "Dump page table", dump_page_table);
    install_command("dump pd", "Dump page directory", dump_page_directory);
    install_command("tmem", "Test memory allocation", test_mem);
    install_command("theap", "Test dynamic heap growth", test_dynamic_heap_growth);
}

static void dump_page_table()
{
    for (int i = 0; i < 10; i++) // Adjust the range as needed
    {
        printf("Page Table[%d]: 0x%x\n", i, page_table[i]);
    }
}

static void test_dynamic_heap_growth()
{
    printf("Initial heap_end: %p\n", heap_end);

    void* block1 = kmalloc(64);
    printf("Allocated block1: %p\n", block1);

    void* large_block = kmalloc(10 * 1024 * 1024); // Allocate 10 MB
    if (large_block)
    {
        printf("Allocated large_block: %p\n", large_block);
        put_hex(ksize(large_block));
        putc('\n');
        put_hex(2 * 1024 * 1024);
        putc('\n');
    }
    else
    {
        puts_color("Failed to allocate large block\n", RED);
    }

    printf("New heap_end: %p\n", heap_end);

    kfree(block1);
    kfree(large_block);

    printf("Heap freed.\n");
}

static void dump_page_directory()
{
    printf("Page Directory:\n");
    for (int i = 0; i < PAGE_DIRECTORY_ENTRIES; i++)
    {
        if (page_directory[i] & 0x1) // Present
        {
            printf("Directory[%d]: 0x%x\n", i, page_directory[i]);
        }
    }
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
    
    void* block2 = kmalloc(HEAP_SIZE - 0x100);
    
    printf("Allocated block2: %p\n", block2);
    // while (1);

    printf("Size of block2: ");
    put_zu(ksize(block2));
    putc('\n');
    put_hex(ksize(block2));
    putc('\n');
    put_hex(HEAP_SIZE - 0x100);
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
