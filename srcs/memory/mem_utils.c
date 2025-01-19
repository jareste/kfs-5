#include "../utils/stdint.h"
#include "../utils/utils.h" // For memset
#include "../utils/stdint.h"
#include "../display/display.h"
#include "../kshell/kshell.h"

#define HEAP_START 0x100000  /* Start of the heap */
#define HEAP_SIZE  0x100000  /* 1 MB heap size */
#define ALIGN(x) (((x) + 7) & ~7) /* 8-byte alignment */

typedef struct block_header {
    size_t size;               /* Size of the block (excluding header) */
    struct block_header* next; /* Pointer to the next free block */
    int free;                  /* Is this block free? (1 for yes, 0 for no) */
} block_header_t;

static block_header_t* free_list = NULL;
static void* heap_end = (void*)HEAP_START;

static void test_mem();

void* kbrk(void* addr)
{
    if ((uintptr_t)addr > HEAP_START + HEAP_SIZE)
    {
        puts("Heap overflow!\n");
        return (void*)-1;
    }

    heap_end = addr;
    return heap_end;
}

void* kmalloc(size_t size)
{
    size = ALIGN(size); /* Ensure size is aligned to 8 bytes */
    block_header_t* current = free_list;
    block_header_t* prev = NULL;

    /* Search for a suitable free block */
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

    /* No suitable block found, expand the heap */
    block_header_t* new_block = (block_header_t*)heap_end;
    if ((void*)((char*)heap_end + sizeof(block_header_t) + size) > (void*)(HEAP_START + HEAP_SIZE))
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

    kbrk((char*)heap_end + sizeof(block_header_t) + size);
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

    install_command("tmem", "Test memory allocation", test_mem);
}

static void test_mem()
{
    // void* block1 = (void*)0x10000C; // Test for SEGV. And it works! (no SEGV)
    void* block1 = kmalloc(64);
    printf("Allocated block1: %p\n", block1);
    printf("Size of block1: %d\n", ksize(block1));
    for (int i = 0; i < 64; i++)
    {
        ((char*)block1)[i] = 'A';
    }
    ((char*)block1)[63] = '\0';
    printf("Block1: %s\n", block1);
    
    void* block2 = kmalloc(0x100000);
    
    printf("Allocated block2: %p\n", block2);
    // while (1);

    printf("Size of block2: %d\n", ksize(block2));

    kfree(block2);

    block2 = kmalloc(0x10000);
    printf("Reallocated block2: %p\n", block2);
    printf("Size of block2: %d\n", ksize(block2));
    kfree(block2);

    kfree(block1);

    printf("Memory freed\n");
}

