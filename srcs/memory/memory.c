#include "../utils/stdint.h"
#include "../display/display.h"
#include "../kshell/kshell.h"
#include "memory.h"
#include "mem_utils.h"
#include "../utils/utils.h"
#include "pmm.h"

extern uint32_t endkernel;

/*############################################################################*/
/*                                                                            */
/*                           DEFINES                                          */
/*                                                                            */
/*############################################################################*/
#define HEAP_START ((uintptr_t)&endkernel + 0x4000) // Start heap 4 KB after kernel
#define HEAP_SIZE_  0x100000  /* 1 MB heap size */
#define ALIGN_4K(x)  (((x) + 0xFFF) & ~0xFFF) /* 4 KB alignment */
#define ALIGN_8(x) (((x) + 0x7) & ~0x7) /* 8-byte alignment */
#define MB(x) (x * 1024 * 1024) /* Convert MB to bytes */
#define KB(x) (x * 1024) /* Convert KB to bytes */
#define MAX_HEAP_SIZE (100 * 1024 * 1024) /* Allow up to 100MB */

/**
 * A simplistic “vmalloc region” from 0xC1000000 up to 0xC2000000
 * (16 MB) just as an example. Make sure it doesn't overlap your main
 * kernel heap or PDE[0] identity region.
 */
#define VMALLOC_START 0xC1000000
#define VMALLOC_END   0xC2000000  // 16 MB region

typedef struct block_header {
    size_t size;               /* Size of the block (excluding header) */
    struct block_header* next; /* Pointer to the next free block */
    int free;                  /* Is this block free? (1 for yes, 0 for no) */
} block_header_t;

typedef struct vblock_header {
    size_t size;                  // total size in bytes of this vblock
    struct vblock_header* next;   // next block in a simple list
    int free;                     // whether it’s free
} vblock_header_t;

typedef uint32_t page_directory_t[PAGE_DIRECTORY_ENTRIES] __attribute__((aligned(PAGE_SIZE)));
typedef uint32_t page_table_t[PAGE_TABLE_ENTRIES] __attribute__((aligned(PAGE_SIZE)));

/*############################################################################*/
/*                                                                            */
/*                           PROTOTYPES                                       */
/*                                                                            */
/*############################################################################*/

static void m_force_page_fault_write();
static void m_force_page_fault_ro();
static void dump_page_table();
static void test_mem();
static void test_dynamic_heap_growth();
static void test_vmalloc();
static void show_allocations();
static void K2();
void dump_page_directory();

/*############################################################################*/
/*                                                                            */
/*                           LOCALS                                           */
/*                                                                            */
/*############################################################################*/
static page_directory_t page_directory __attribute__((aligned(PAGE_SIZE)));
static page_table_t page_table __attribute__((aligned(PAGE_SIZE)));

static block_header_t* free_list = NULL;
static void* heap_end;
static size_t HEAP_SIZE;

static vblock_header_t* vblock_list = NULL; // head of a linked list
static uintptr_t vheap_end = VMALLOC_START; // used by vbrk (similar to vmalloc_next)

static command_t commands[] = {
    {"f pfw", "Force a page fault by writing to an unmapped address", m_force_page_fault_write},
    {"f pfro", "Force a page fault by writing to a read-only page", m_force_page_fault_ro},
    {"dump pt", "Dump page table", dump_page_table},
    {"dump pd", "Dump page directory", dump_page_directory},
    {"tmem", "Test memory allocation", test_mem},
    {"theap", "Test dynamic heap growth", test_dynamic_heap_growth},
    {"vmalloc", "Test vmalloc", test_vmalloc},
    {"mem2", "Allocate 2 MB. No Free", K2},
    {"show alloc", "Show allocated memory", show_allocations},
    {NULL, NULL, NULL}
};

/*############################################################################*/
/*                                                                            */
/*                           FUNCTIONS                                        */
/*                                                                            */
/*############################################################################*/

void paging_init()
{
    pmm_init();
    memset(page_directory, 0, sizeof(page_directory));
    memset(page_table, 0, sizeof(page_table));

    /* Map the first 4MB (first page table) */
    for (int i = 0; i < PAGE_TABLE_ENTRIES; i++)
    {
        page_table[i] = (i * PAGE_SIZE) | 0x03; /* FLAGS: Present, Read/Write */
    }

    page_directory[0] = ((uint32_t)page_table) | 0x03; /* FLAGS: Present, Read/Write */

    /* Load page directory */
    __asm__ __volatile__("mov %0, %%cr3" : : "r"(page_directory));

    /* Enable paging */
    uint32_t cr0;
    __asm__ __volatile__("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000; // Enable paging
    __asm__ __volatile__("mov %0, %%cr0" : : "r"(cr0));
}

void heap_init()
{
    heap_end = (void*)ALIGN_4K((uintptr_t)HEAP_START);
    free_list = (block_header_t*)heap_end;
    free_list->size = HEAP_SIZE_ - sizeof(block_header_t);
    free_list->next = NULL;
    free_list->free = 1;


    printf("Kernel ends at: %p\n", &endkernel);
    printf("Heap starts at: %p\n", HEAP_START);
    printf("Heap size: %z bytes\n", HEAP_SIZE_);

    install_all_cmds(commands);
}

void* kbrk(void* addr)
{
    if ((uintptr_t)addr > HEAP_START + MAX_HEAP_SIZE)
    {
        puts_color("WARNING: Heap exceeds maximum limit!\n", RED);
        return (void*)-1;
    }

    uint32_t new_heap_end = ALIGN_4K((uintptr_t)addr);
    uint32_t current_heap_end = ALIGN_4K((uintptr_t)heap_end);

    while (current_heap_end < new_heap_end)
    {
        uint32_t page_directory_index = current_heap_end / (PAGE_SIZE * PAGE_TABLE_ENTRIES);
        uint32_t page_table_index = (current_heap_end % (PAGE_SIZE * PAGE_TABLE_ENTRIES)) / PAGE_SIZE;

        if (!(page_directory[page_directory_index] & 0x1))
        {
            uint32_t pt_phys = allocate_frame();
            if (!pt_phys) { kernel_panic("Failed to allocate frame for new page table"); }

            page_directory[page_directory_index] = pt_phys | (PAGE_PRESENT | PAGE_RW);


        }

        /* Map the page in the page table */
        page_table_t* table = (page_table_t*)(page_directory[page_directory_index] & ~0xFFF);
        if (!((*table)[page_table_index] & 0x1))
        {
            (*table)[page_table_index] = current_heap_end | 0x03; // Present + RW
        }

        current_heap_end += PAGE_SIZE;
    }

    heap_end = addr;
    return heap_end;

}

/*############################################################################*/
/*                                                                            */
/*                           KMALLOC                                          */
/*                                                                            */
/*############################################################################*/

/**
 * kmalloc:
 *   Allocates a contiguous block of memory of at least `size` bytes
 *   from the kernel “heap.” It tries to find a free block from the free_list;
 *   if none are large enough, it expands the heap via `kbrk()`.
 */
void* kmalloc(size_t size)
{
    size = ALIGN_8(size);

    block_header_t* current = free_list;
    block_header_t* prev = NULL;

    while (current)
    {
        if (current->free && current->size >= size)
        {
            current->free = 0;

            /* Split the block if it's considerably larger than ‘size’ */
            if (current->size > size + sizeof(block_header_t))
            {
                block_header_t* new_block =
                    (block_header_t*)((char*)current + sizeof(block_header_t) + size);

                /* If this split goes beyond current heap_end, we expand */
                uintptr_t new_block_end = (uintptr_t)new_block + sizeof(block_header_t);
                if (new_block_end > (uintptr_t)heap_end)
                {
                    /* Expand heap with kbrk */
                    if (kbrk((void*)(new_block_end)) == (void*)-1)
                    {
                        puts_color("kmalloc: Out of memory during block split!\n", RED);
                        current->free = 1;
                        return NULL;
                    }
                }

                /* Set up the new block */
                new_block->size = current->size - size - sizeof(block_header_t);
                new_block->free = 1;
                new_block->next = current->next;

                /* Adjust current */
                current->size = size;
                current->next = new_block;
            }

            /* Return a pointer after the header */
            void* allocated_mem = (char*)current + sizeof(block_header_t);
            return allocated_mem;
        }

        prev = current;
        current = current->next;
    }

    /* No suitable free block, so we must expand the heap */
    block_header_t* new_block = (block_header_t*)heap_end;
    uintptr_t new_heap_end = (uintptr_t)new_block + sizeof(block_header_t) + size;
    if (kbrk((void*)new_heap_end) == (void*)-1)
    {
        puts_color("kmalloc: Out of memory (heap expansion)!\n", RED);
        return NULL;
    }

    /* Initialize the newly created block */
    new_block->size = size;
    new_block->free = 0;
    new_block->next = NULL;

    if (prev)
    {
        prev->next = new_block;
    }
    else
    {
        /* first block in the list */
        free_list = new_block;
    }

    void* allocated_mem = (char*)new_block + sizeof(block_header_t);
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


/*############################################################################*/
/*                                                                            */
/*                           VMALLOC                                          */
/*                                                                            */
/*############################################################################*/

static inline uint32_t make_pde_flags()
{
    return PAGE_PRESENT | PAGE_RW; // TODO: we can set PAGE_USER here
}
static inline uint32_t make_pte_flags()
{
    return PAGE_PRESENT | PAGE_RW;
}

/**
 * create_page_table_if_needed:
 *   If PDE at pd_index not present, allocate a frame for that page table,
 *   zero it, set PDE.  Then return pointer to the page table.
 */
static page_table_t* create_page_table_if_needed(uint32_t pd_index)
{
    if (!(page_directory[pd_index] & PAGE_PRESENT))
    {
        uint32_t pt_phys = allocate_frame();
        if (!pt_phys)
        {
            puts_color("vmalloc: out of frames for new page table!\n", RED);
            return NULL; 
        }
        page_directory[pd_index] = pt_phys | make_pde_flags();
    }
    uint32_t pt_base = page_directory[pd_index] & ~0xFFF;
    return (page_table_t*)pt_base;
}

static void map_new_page(uintptr_t vaddr)
{
    uint32_t pd_index = vaddr >> 22;
    uint32_t pt_index = (vaddr >> 12) & 0x3FF;

    page_table_t* pt = create_page_table_if_needed(pd_index);
    if (!pt)
    {
        puts_color("map_new_page: PDE creation failed!\n", RED);
        return;
    }

    uint32_t frame_phys = allocate_frame();
    if (!frame_phys)
    {
        puts_color("map_new_page: out of physical frames!\n", RED);
        return;
    }
    (*pt)[pt_index] = (frame_phys & ~0xFFF) | make_pte_flags();
    asm volatile("invlpg (%0)" : : "r"(vaddr) : "memory");
}

static void unmap_page(uintptr_t vaddr)
{
    uint32_t pd_index = vaddr >> 22;
    uint32_t pt_index = (vaddr >> 12) & 0x3FF;

    /* PDE exists? */
    if (page_directory[pd_index] & PAGE_PRESENT)
    {
        page_table_t* pt = (page_table_t*)(page_directory[pd_index] & ~0xFFF);
        if ((*pt)[pt_index] & PAGE_PRESENT)
        {
            /* Free the frame */
            uint32_t phys = (*pt)[pt_index] & ~0xFFF;
            free_frame(phys);

            /* Clear PTE */
            (*pt)[pt_index] = 0;
            asm volatile("invlpg (%0)" : : "r"(vaddr) : "memory");
        }
    }
    /* TODO: Free the page if it's not used at all. */
}

void* vbrk(void* addr)
{
    uintptr_t new_end = (uintptr_t)addr;
    if (new_end < VMALLOC_START || new_end > VMALLOC_END)
    {
        puts_color("vbrk: out of vmalloc region!\n", RED);
        return (void*)-1;
    }

    if (new_end > vheap_end)
    {
        /* ask for more vmem */
        for (uintptr_t p = vheap_end; p < new_end; p += PAGE_SIZE)
        {
            map_new_page(p);
        }
    }
    else if (new_end < vheap_end)
    {
        /* We got less vmem, so lets make it shorter. */
        for (uintptr_t p = new_end; p < vheap_end; p += PAGE_SIZE)
        {
            unmap_page(p);
        }
    }

    vheap_end = new_end;
    return (void*)vheap_end;
}

void* vmalloc(size_t size)
{
    size = ALIGN_8(size);
    size_t total_size = size + sizeof(vblock_header_t);

    uintptr_t old_end = vheap_end;
    uintptr_t new_end = old_end + total_size;
    if (new_end > VMALLOC_END)
    {
        puts_color("vmalloc: out of range!\n", RED);
        return NULL;
    }

    if (vbrk((void*)new_end) == (void*)-1)
    {
        puts_color("vmalloc: vbrk failed\n", RED);
        return NULL;
    }

    vblock_header_t* block = (vblock_header_t*)old_end;
    block->size = size;
    block->free = 0;
    block->next = vblock_list;
    vblock_list = block;

    void* ret = (void*)((uintptr_t)block + sizeof(vblock_header_t));
    return ret;
}

size_t vsize(void* ptr)
{
    if (!ptr) return 0;
    vblock_header_t* block = (vblock_header_t*)((char*)ptr - sizeof(vblock_header_t));
    return block->size;
}

void vfree(void* ptr)
{
    if (!ptr) return;
    vblock_header_t* block = (vblock_header_t*)((char*)ptr - sizeof(vblock_header_t));
    size_t size = block->size;
    block->free = 1;

    uintptr_t start = (uintptr_t)block;
    uintptr_t end   = start + sizeof(vblock_header_t) + size;
    start &= ~0xFFF;
    end   = (end + 0xFFF) & ~0xFFF;

    for (uintptr_t addr = start; addr < end; addr += PAGE_SIZE)
    {
        unmap_page(addr);
    }
}

/*############################################################################*/
/*                                                                            */
/*                           TESTS                                            */
/*                                                                            */
/*############################################################################*/

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

    // page_table_t* table = (page_table_t*)(page_directory[0] & ~0xFFF);

    // printf("Table[%d][%d]: %x\n", 0, 0, (*table)[0]);

}

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

void K2()
{
    kmalloc(2 * 1024 * 1024);
}

static void test_dynamic_heap_growth()
{
    printf("Initial heap_end: %p\n", heap_end);

    void* block1 = kmalloc(64);
    printf("Allocated block1: %p\n", block1);


    size_t malloc_size = MB(99);
    void* large_block = kmalloc(malloc_size); // Allocate 10 MB
    // void* large_block = kmalloc(2); // Allocate 10 MB
    // large_block = kmalloc(2); // Allocate 10 MB
    
    // void* large_block = kmalloc(0x200000); // Allocate 10 MB
    if (large_block)
    {
        printf("Allocated large_block: %p\n", large_block);
        put_hex(ksize(large_block));
        putc('\n');
        put_hex(malloc_size);
        putc('\n');
        for (int i = 0; i < malloc_size; i++)
        {
            ((char*)large_block)[i] = 'A';
        }
        ((char*)large_block)[malloc_size - 1] = '\0';
        puts_color(large_block + (malloc_size - 10), GREEN);
        putc('\n');
    }
    else
    {
        puts_color("WARNING: Failed to allocate large block\n", RED);
    }

    // printf("Heap_end: %p\n", heap_end);
    // void* test = kmalloc(0x100000); // Allocate 4 KB

    // printf("Allocated test: %p, size: 0x", test);
    // put_hex(ksize(test));
    // putc('\n');

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

static void show_allocations()
{
    block_header_t* current = free_list;
    printf("Heap Allocations:\n");
    while (current)
    {
        printf("Block at %p: size = %zu, free = %d\n", (void*)current, current->size, current->free);
        current = current->next;
    }
}

static void test_vmalloc()
{
    size_t size = MB(5);
    void* vm1 = vmalloc(size);
    printf("Allocated vm1: %p\n", vm1);
    printf("Size of vm1: %z\n", vsize(vm1));
    printf("Size of vm1: %z\n", size);
    vfree(vm1);
}
