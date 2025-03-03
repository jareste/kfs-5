#include "../utils/stdint.h"
#include "../display/display.h"
#include "../kshell/kshell.h"
#include "memory.h"
#include "../utils/utils.h"
#include "pmm.h"

extern uint32_t endkernel;

/*############################################################################*/
/*                                                                            */
/*                           DEFINES                                          */
/*                                                                            */
/*############################################################################*/
// #define HEAP_START ((uintptr_t)&endkernel + 0x4000) // Start heap 4 KB after kernel
#define HEAP_START (ALIGN_4K((uintptr_t)&endkernel + 0x4000))
#define HEAP_SIZE_  0x100000  /* 1 MB heap size */
#define ALIGN_4K(x)  (((x) + 0xFFF) & ~0xFFF) /* 4 KB alignment */
#define ALIGN_8(x)   (((x) + 0x7)   & ~0x7)   /* 8-byte alignment */
/* Maximum allowed heap expansion */
#define MAX_HEAP_SIZE (4 * 1024 * 1024) /* 4 MB */

/* 
 * Reserve a separate virtual region for vmalloc (immediately after the kernel heap).
 */
#define VMALLOC_START (ALIGN_4K((uintptr_t)HEAP_START + MAX_HEAP_SIZE))
#define VMALLOC_SIZE  (16 * 1024 * 1024)   /* 16 MB vmalloc region */

#define PAGE_PRESENT  0x1
#define PAGE_RW       0x2
#define PAGE_USER     0x4

#define KERNEL_PDE_FLAGS  (PAGE_PRESENT | PAGE_RW)
#define KERNEL_PTE_FLAGS  (PAGE_PRESENT | PAGE_RW)

#define USER_PDE_FLAGS  (PAGE_PRESENT | PAGE_RW | PAGE_USER)
#define USER_PTE_FLAGS  (PAGE_PRESENT | PAGE_RW | PAGE_USER)

typedef struct block_header {
    size_t size;               /* Size of the block (excluding header) */
    struct block_header* next; /* Pointer to the next free block */
    int free;                  /* Is this block free? (1 for yes, 0 for no) */
} block_header_t;

typedef struct vblock_header {
    size_t size;               /* Size of the block (excluding header) */
    struct vblock_header* next; /* Pointer to the next free block */
    int free;                  /* Is this block free? (1 for yes, 0 for no) */
    int is_user;               /* Is this block for user space? (1 for yes, 0 for no) */
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
static void K2();
static void show_user_allocations();
static void show_kernel_allocations();
static void test_kmalloc();
void dump_page_directory();

/*############################################################################*/
/*                                                                            */
/*                           LOCALS                                           */
/*                                                                            */
/*############################################################################*/
static page_directory_t page_directory __attribute__((aligned(PAGE_SIZE)));

/* Heap for kmalloc */
static block_header_t* free_list = NULL;
static block_header_t* heap_started;
static void* heap_end;
static size_t HEAP_SIZE;

/* A separate free list for vmalloc blocks */
static vblock_header_t* vblock_list = NULL;
/* Bump pointer for new vmalloc mappings */
static void* vmalloc_brk;

static command_t commands[] = {
    {"f pfw", "Force a page fault by writing to an unmapped address", m_force_page_fault_write},
    {"f pfro", "Force a page fault by writing to a read-only page", m_force_page_fault_ro},
    {"dump pt", "Dump page table", dump_page_table},
    {"dump pd", "Dump page directory", dump_page_directory},
    {"tmem", "Test memory allocation", test_mem},
    {"theap", "Test dynamic heap growth", test_dynamic_heap_growth},
    {"vmalloc", "Test vmalloc", test_vmalloc},
    {"kmalloc", "Test kmalloc", test_kmalloc},
    {"mem2", "Allocate 2 MB. No Free", K2},
    {"show alloc", "Show allocated memory", show_kernel_allocations},
    {"show user", "Show user allocated memory", show_user_allocations},
    {NULL, NULL, NULL}
};

/*############################################################################*/
/*                                                                            */
/*                           FUNCTIONS                                        */
/*                                                                            */
/*############################################################################*/

static void map_page(uintptr_t virt_addr, uint32_t phys_addr, uint32_t flags)
{
    uint32_t pd_index = virt_addr >> 22;
    uint32_t pt_index = (virt_addr >> 12) & 0x3FF;

    if (!(page_directory[pd_index] & PAGE_PRESENT)) {
        uint32_t pt_frame = allocate_frame();
        if (!pt_frame)
            kernel_panic("map_page: Out of frames for PDE!\n");

        memset((void*)pt_frame, 0, PAGE_SIZE);
        /* Pass along the PAGE_USER flag if needed */
        uint32_t pde_flags = (pt_frame & ~0xFFF) | (flags & PAGE_USER) | PAGE_PRESENT | PAGE_RW;
        page_directory[pd_index] = pde_flags;
    }

    page_table_t* pt = (page_table_t*)(page_directory[pd_index] & ~0xFFF);
    (*pt)[pt_index] = (phys_addr & ~0xFFF) | (flags & 0xFFF);

    /* Flush TLB for this page */
    asm volatile("invlpg (%0)" :: "r"(virt_addr) : "memory");
}

void paging_init()
{
    pmm_init();
    memset(page_directory, 0, sizeof(page_directory));

    /* Identity-map the first 64MB (4KB per mapping) */
    const uint32_t IDENTITY_LIMIT = MB(64);
    for (uintptr_t addr = 0; addr < IDENTITY_LIMIT; addr += 0x1000)
    {
        uint32_t pd_index = addr >> 22;
        uint32_t pt_index = (addr >> 12) & 0x3FF;

        if (!(page_directory[pd_index] & PAGE_PRESENT))
        {
            uint32_t pt_phys = allocate_frame();
            if (!pt_phys) kernel_panic("paging_init: out of frames for PDE\n");

            memset((void*)pt_phys, 0, PAGE_SIZE);
            page_directory[pd_index] = (pt_phys & ~0xFFF) | (PAGE_PRESENT | PAGE_RW);
        }

        page_table_t* table = (page_table_t*)(page_directory[pd_index] & ~0xFFF);
        (*table)[pt_index] = (addr & ~0xFFF) | (PAGE_PRESENT | PAGE_RW);
    }

    asm volatile("mov %0, %%cr3" :: "r"(page_directory));

    /* Enable paging (set PG bit in CR0) */
    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;
    asm volatile("mov %0, %%cr0" :: "r"(cr0));
}

void heap_init()
{
    /* Initialize kmalloc heap */
    heap_end = (void*)ALIGN_4K((uintptr_t)HEAP_START);
    free_list = NULL;
    heap_started = NULL;

    /* Initialize vmalloc arena */
    vmalloc_brk = (void*)VMALLOC_START;
    vblock_list = NULL;

    install_all_cmds(commands, MEMORY);
}

// static void map_page_kernel(uintptr_t virt, uint32_t phys)
// {
//     map_page(virt, phys, PAGE_PRESENT | PAGE_RW);
// }

// static void map_page_user(uintptr_t virt, uint32_t phys)
// {
//     map_page(virt, phys, PAGE_PRESENT | PAGE_RW | PAGE_USER);
// }

/*############################################################################*/
/*                                                                            */
/*                           KMALLOC                                          */
/*                                                                            */
/*############################################################################*/

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
        uint32_t pd_index = current_heap_end >> 22;
        uint32_t pt_index = (current_heap_end >> 12) & 0x3FF;

        if (!(page_directory[pd_index] & PAGE_PRESENT))
        {
            uint32_t pt_phys = allocate_frame();
            if (!pt_phys) kernel_panic("kbrk: Failed to allocate page table frame!");

            memset((void*)pt_phys, 0, PAGE_SIZE);
            page_directory[pd_index] = pt_phys | (PAGE_PRESENT | PAGE_RW);
        }

        page_table_t* table = (page_table_t*)(page_directory[pd_index] & ~0xFFF);
        if (!((*table)[pt_index] & PAGE_PRESENT))
        {
            uint32_t phys_frame = allocate_frame();
            if (!phys_frame) kernel_panic("kbrk: Failed to allocate frame!");

            (*table)[pt_index] = phys_frame | (PAGE_PRESENT | PAGE_RW);
        }

        current_heap_end += PAGE_SIZE;
    }

    heap_end = (void*)new_heap_end;
    return heap_end;
}

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
            /* Split block if there is extra space for a new block header plus minimal payload */
            if (current->size > size + sizeof(block_header_t) + 8)
            {
                block_header_t* new_block = (block_header_t*)((uintptr_t)current + sizeof(block_header_t) + size);
                new_block->size = current->size - size - sizeof(block_header_t);
                new_block->free = 1;
                new_block->next = current->next;
                current->size = size;
                current->next = new_block;
            }
            return (void*)((uintptr_t)current + sizeof(block_header_t));
        }
        prev = current;
        current = current->next;
    }

    /* No free block found; expand the heap */
    uintptr_t old_end = (uintptr_t)heap_end;
    size_t new_size = size + sizeof(block_header_t);
    uintptr_t new_heap_end = (uintptr_t)heap_end + new_size;
    if (new_heap_end > HEAP_START + MAX_HEAP_SIZE)
    {
        puts_color("kmalloc: Out of memory (heap expansion)!\n", RED);
        return NULL;
    }

    if (kbrk((void*)new_heap_end) == (void*)-1)
    {
        puts_color("kmalloc: Failed to expand heap!\n", RED);
        return NULL;
    }

    block_header_t* new_block = (block_header_t*)old_end;
    new_block->size = size;
    new_block->free = 0;
    new_block->next = NULL;

    if (prev)
        prev->next = new_block;
    else
    {
        free_list = new_block;
        heap_started = free_list;
    }

    return (void*)((uintptr_t)new_block + sizeof(block_header_t));
}

void kfree(void* ptr)
{
    if (!ptr) return;
    block_header_t* block = (block_header_t*)((uintptr_t)ptr - sizeof(block_header_t));
    block->free = 1;

    /* Merge with next block if contiguous */
    if (block->next && block->next->free)
    {
        uintptr_t block_end = (uintptr_t)block + sizeof(block_header_t) + block->size;
        if (block_end == (uintptr_t)block->next)
        {
            block->size += sizeof(block_header_t) + block->next->size;
            block->next = block->next->next;
        }
    }

    /* Merge with previous block if contiguous */
    block_header_t* current = heap_started;
    block_header_t* prev = NULL;
    while (current && current != block)
    {
        prev = current;
        current = current->next;
    }
    if (prev && prev->free)
    {
        uintptr_t prev_end = (uintptr_t)prev + sizeof(block_header_t) + prev->size;
        if (prev_end == (uintptr_t)block)
        {
            prev->size += sizeof(block_header_t) + block->size;
            prev->next = block->next;
        }
    }
}

size_t ksize(void* ptr)
{
    if (!ptr) return 0;

    block_header_t* block = (block_header_t*)((uintptr_t)ptr - sizeof(block_header_t));
    return block->size;
}

/*############################################################################*/
/*                                                                            */
/*                           VMALLOC                                          */
/*                                                                            */
/*############################################################################*/

// void* vmalloc(size_t size, int is_user)
// {
//     /* Round up payload size and add header */
//     size_t aligned_size = ALIGN_4K(size);
//     size_t total_size = ALIGN_4K(aligned_size + sizeof(vblock_header_t));

//     vblock_header_t* current = vblock_list;
//     vblock_header_t* prev = NULL;
//     while (current)
//     {
//         if (current->free && current->size >= total_size)
//         {
//             current->free = 0;
//             current->is_user = is_user;
//             /* Split block if possible */
//             if (current->size > total_size + sizeof(vblock_header_t) + PAGE_SIZE)
//             {
//                 vblock_header_t* new_block = (vblock_header_t*)((uintptr_t)current + total_size);
//                 new_block->size = current->size - total_size;
//                 new_block->free = 1;
//                 new_block->next = current->next;
//                 new_block->is_user = 0;
//                 current->size = total_size;
//                 current->next = new_block;
//             }
//             return (void*)((uintptr_t)current + sizeof(vblock_header_t));
//         }
//         prev = current;
//         current = current->next;
//     }

//     uintptr_t old_brk = (uintptr_t)vmalloc_brk;
//     uintptr_t new_brk = old_brk + total_size;
//     if (new_brk > VMALLOC_START + VMALLOC_SIZE)
//     {
//         puts_color("vmalloc: Out of vmalloc space!\n", RED);
//         return NULL;
//     }

//     /* Map each page in the new region */
//     for (uintptr_t addr = old_brk; addr < new_brk; addr += PAGE_SIZE)
//     {
//         uint32_t phys_frame = allocate_frame();
//         if (!phys_frame)
//         {
//             puts_color("vmalloc: Out of physical frames!\n", RED);
//             return NULL;
//         }
//         uint32_t flags = PAGE_PRESENT | PAGE_RW;
//         if (is_user)
//             flags |= PAGE_USER;
//         map_page(addr, phys_frame, flags);
//     }
//     vmalloc_brk = (void*)new_brk;

//     /* Create the new block header */
//     vblock_header_t* new_block = (vblock_header_t*)old_brk;
//     new_block->size = total_size;
//     new_block->free = 0;
//     new_block->next = NULL;
//     new_block->is_user = is_user;

//     if (prev)
//         prev->next = new_block;
//     else
//         vblock_list = new_block;

//     return (void*)((uintptr_t)new_block + sizeof(vblock_header_t));
// }

void* vmalloc(size_t size, int is_user)
{
    size = ALIGN_8(size);

    vblock_header_t* current = vblock_list;
    vblock_header_t* prev = NULL;

    while (current)
    {
        if (current->free && current->size >= size)
        {
            current->free = 0;
            current->is_user = is_user;

            if (current->size > size + sizeof(vblock_header_t))
            {
                vblock_header_t* new_block = (vblock_header_t*)ALIGN_8((uintptr_t)current + sizeof(vblock_header_t) + size);
                new_block->size = current->size - size - sizeof(vblock_header_t);
                new_block->free = 1;
                new_block->next = current->next;
                new_block->is_user = is_user;

                current->size = size;
                current->next = new_block;
            }

            void* allocated_mem = (char*)current + sizeof(vblock_header_t);
            return allocated_mem;
        }

        prev = current;
        current = current->next;
    }

    uintptr_t old_end = (uintptr_t)heap_end;
    size_t new_size = size + sizeof(vblock_header_t);
    uintptr_t new_heap_end = (uintptr_t)heap_end + new_size;

    if (new_heap_end > HEAP_START + MAX_HEAP_SIZE)
    {
        puts_color("vmalloc: Out of memory (heap expansion)!\n", RED);
        return NULL;
    }

    if (kbrk((void*)new_heap_end) == (void*)-1)
    {
        puts_color("vmalloc: Failed to expand heap!\n", RED);
        return NULL;
    }

    vblock_header_t* new_block = (vblock_header_t*)old_end;
    new_block->size = size;
    new_block->free = 0;
    new_block->next = NULL;
    new_block->is_user = is_user;

    if (prev)
    {
        prev->next = new_block;
    }
    else
    {
        vblock_list = new_block;
    }

    void* allocated_mem = (char*)new_block + sizeof(vblock_header_t);
    return allocated_mem;
}

void vfree(void* ptr)
{
    if (!ptr) return;
    vblock_header_t* block = (vblock_header_t*)((uintptr_t)ptr - sizeof(vblock_header_t));
    block->free = 1;

    vblock_header_t* current = vblock_list;
    vblock_header_t* prev = NULL;
    while (current)
    {
        if (current->free)
        {
            /* Merge with next block if physically contiguous */
            if (current->next && current->next->free)
            {
                uintptr_t current_end = (uintptr_t)current + sizeof(vblock_header_t) + current->size;
                if (current_end == (uintptr_t)current->next)
                {
                    current->size += sizeof(vblock_header_t) + current->next->size;
                    current->next = current->next->next;
                    continue; // re-check current->next
                }
            }
            /* Merge with previous block if physically contiguous */
            if (prev && prev->free)
            {
                uintptr_t prev_end = (uintptr_t)prev + sizeof(vblock_header_t) + prev->size;
                if (prev_end == (uintptr_t)current)
                {
                    prev->size += sizeof(vblock_header_t) + current->size;
                    prev->next = current->next;
                    current = prev; // set current to merged block
                    continue;
                }
            }
        }
        prev = current;
        current = current->next;
    }
}

size_t vsize(void* ptr)
{
    if (!ptr) return 0;
    vblock_header_t* block = (vblock_header_t*)((uintptr_t)ptr - sizeof(vblock_header_t));
    return block->size;
}

/*############################################################################*/
/*                                                                            */
/*                           MMAP                                             */
/*                                                                            */
/*############################################################################*/
static int region_is_free_in_vblocks(uintptr_t start, size_t length)
{
    uintptr_t end = start + length;
    vblock_header_t* current = vblock_list;

    while (current)
    {
        if (!current->free)
        {
            uintptr_t block_start = (uintptr_t)current + sizeof(vblock_header_t);
            uintptr_t block_end   = block_start + current->size;

            if (end > block_start && start < block_end)
            {
                return 0;
            }
        }
        current = current->next;
    }
    return 1;
}

static void insert_fixed_vblock(uintptr_t start, size_t length, int is_user)
{
    vblock_header_t* block = (vblock_header_t*)start;

    block->size    = length - sizeof(vblock_header_t);
    block->free    = 0;
    block->is_user = is_user;

    block->next = vblock_list;
    vblock_list = block;
}


void* mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset)
{
    if (!(flags & MAP_ANONYMOUS))
    {
        puts_color("mmap: Only MAP_ANONYMOUS is supported!\n", RED);
        return (void*)-1;
    }

    int is_user = (prot & PROT_USER) ? 1 : 0;

    if (flags & MAP_FIXED)
    {
        uintptr_t fixed_addr   = ALIGN_4K((uintptr_t)addr);
        size_t aligned_length  = ALIGN_4K(length);
        uintptr_t fixed_end    = fixed_addr + aligned_length;

        if (!region_is_free_in_vblocks(fixed_addr, aligned_length))
        {
            puts_color("mmap: region is NOT free!\n", RED);
            return (void*)-1;
        }

        uint32_t page_flags = PAGE_PRESENT;
        if (prot & PROT_WRITE) page_flags |= PAGE_RW;
        if (prot & PROT_USER)  page_flags |= PAGE_USER;

        for (uintptr_t va = fixed_addr; va < fixed_end; va += PAGE_SIZE)
        {
            uint32_t frame = allocate_frame();
            if (!frame)
            {
                puts_color("mmap: out of physical frames!\n", RED);
                return (void*)-1;
            }
            map_page(va, frame, page_flags);
        }

        insert_fixed_vblock(fixed_addr, aligned_length, is_user);

        return (void*)fixed_addr;
    }
    else
    {
        void* res = vmalloc(length, is_user);
        if (!res)
        {
            puts_color("mmap: vmalloc failed!\n", RED);
            return (void*)-1;
        }
        return res;
    }
}

int munmap(void* addr, size_t length)
{
    if (!addr) return -1;

    vfree(addr);
    return 0;
}


void make_page_user(uintptr_t addr)
{
    uint32_t pd_index = addr >> 22;
    uint32_t pt_index = (addr >> 12) & 0x3FF;
    if (page_directory[pd_index] & PAGE_PRESENT)
    {
        page_directory[pd_index] |= PAGE_USER;
        page_table_t* pt = (page_table_t*)(page_directory[pd_index] & ~0xFFF);
        (*pt)[pt_index] |= PAGE_USER;
        asm volatile("invlpg (%0)" :: "r"(addr) : "memory");
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
    puts_color("Maybe not working\n", RED);
    __asm__ __volatile__("invlpg (%0)" : : "r"(0x1000) : "memory"); // Invalidate TLB for 0x1000
    uint32_t *read_only_addr = (uint32_t*)0x1000;
    *read_only_addr = 0xDEADBEEF; // Attempt to write
}

static void print_pde_flags(uint32_t entry)
{
    putc('[');

    if (entry & PAGE_PRESENT)
        putc('P');
    else
        putc('-');
    
    if (entry & PAGE_RW)
        putc('W');
    else
        putc('R');

    if (entry & PAGE_USER)
        putc('U');
    else
        putc('K');

    putc(']');
}

void dump_page_directory()
{
    printf("Page Directory:\n");
    for (int i = 0; i < PAGE_DIRECTORY_ENTRIES; i++)
    {
        if (page_directory[i] & PAGE_PRESENT)
        {
            uint32_t pde_flags = page_directory[i] & 0xFFF;     // low 12 bits
            uint32_t pde_base  = page_directory[i] & 0xFFFFF000; // top 20 bits

            printf("PDE[%d]: base=%x, flags=", i, pde_base);
            print_pde_flags(pde_flags);
            putc('\n');

            // It just floods the screen so much
            // page_table_t* table = (page_table_t*)pde_base;
            // for (int j = 0; j < PAGE_TABLE_ENTRIES; j++)
            // {
            //     if ((*table)[j] & PAGE_PRESENT)
            //     {
            //         uint32_t pte_val   = (*table)[j];
            //         uint32_t pte_flags = pte_val & 0xFFF;
            //         uint32_t pte_base  = pte_val & 0xFFFFF000;

            //         // For brevity, only print the PTE if you want 
            //         // or only if PDE i == 0 or something.  That might be 
            //         // a lot of output for the entire 1024 PDE * 1024 PTE.

            //         printf("  PTE[%4d]: base=0x%08x, flags=", j, pte_base);
            //         print_pde_flags(pte_flags);
            //         puts("");
            //     }
            // }
        }
        else
        {
            // PDE not present
            if (i < 3) // just to reduce spam
                printf("PDE[%d]: 0x%x [Not present]\n", i, page_directory[i]);
        }
    }
}

static void dump_page_table(uint32_t directory_index)
{
    puts_color("Test not working as intended as it's been modified\n", RED);
    debug_page_mapping(directory_index);
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

/* A big test. */
static void test_dynamic_heap_growth()
{
    size_t size;
    void* block1;
    void* large_block;
    size_t i;

    printf("Initial heap_end: %p\n", heap_end);

    size = 64;
    block1 = kmalloc(size);
    printf("Allocated block1: %p\n", block1);

    memset(block1, 'A', size);
    for (i = 0; i < size; i++)
    {
        if (((char*)block1)[i] != 'A')
        {
            puts_color("Memory corruption detected in block1!\n", RED);
            break;
        }
    }

    kfree(block1);

    size = KB(3);
    large_block = kmalloc(size);
    if (large_block)
    {
        printf("Allocated large_block: %p\n", large_block);

        memset(large_block, 'B', size);
        for (i = 0; i < size; i++)
        {
            if (((char*)large_block)[i] != 'B')
            {
                puts_color("Memory corruption detected in large_block!\n", RED);
                break;
            }
        }
    }
    else
    {
        puts_color("WARNING: Failed to allocate large block\n", RED);
    }
    kfree(large_block);

    printf("New heap_end: %p\n", heap_end);
}


static void test_mem()
{
    void* block1;
    void* block2;

    block1 = kmalloc(64);
    printf("Allocated block1: %p (size=%z)\n", block1, ksize(block1));

    /* Fill it with data */
    memset(block1, 'A', 64);

    /* Free it */
    kfree(block1);

    /* Another test: 2 MB */
    block2 = kmalloc(2 * 1024 * 1024);
    printf("Allocated block2: %p (size=%z)\n", block2, ksize(block2));
    kfree(block2);

    /* Another test: reallocate block2 with 64 KB */
    block2 = kmalloc(0x10000);
    printf("Reallocated block2: %p (size=%z)\n", block2, ksize(block2));
    kfree(block2);

    /* Another small block */
    block2 = kmalloc(0x10);
    printf("Reallocated block2: %p (size=%z)\n", block2, ksize(block2));
    kfree(block2);

    printf("Memory freed.\n");
}

void debug_page_mapping(uint32_t address)
{
    uint32_t pd_index = address >> 22;
    uint32_t pt_index = (address >> 12) & 0x3FF;
    page_table_t* pt;

    printf("\nDebug mapping for address %x\n", address);
    printf(" PDE index: %d, PDE entry: %x\n", 
            pd_index, page_directory[pd_index]);

    if (page_directory[pd_index] & PAGE_PRESENT)
    {
        pt = (page_table_t*)(page_directory[pd_index] & ~0xFFF);
        printf("  PTE index: %d, PTE entry: 0x%x\n", 
                pt_index, (*pt)[pt_index]);
    }
    else
    {
        printf(" PDE not present.\n");
    }
}

static void test_kmalloc()
{
    size_t size;
    int i;
    void* vm;
    void* vm1;
    void* vm2;

    for (i = 0; i < 500; i++)
    {
        size = KB(1);

        vm = kmalloc(size);
        if (!vm)
        {
            set_putchar_color(RED);
            printf("kmalloc: failed to allocate %z bytes\n", size);
            set_putchar_color(LIGHT_GREY);
            return;
        }
        memset(vm, 'A', size);
        vm2 = kmalloc(size);

        if (!vm2)
        {
            set_putchar_color(RED);
            printf("kmalloc: failed to allocate %z bytes\n", size);
            set_putchar_color(LIGHT_GREY);
            return;
        }
        memset(vm2, 'A', size);
        if (memcmp(vm, vm2, size) != 0)
        {
            set_putchar_color(RED);
            printf("kmalloc: memory corruption detected!\n");
            set_putchar_color(LIGHT_GREY);
            kernel_panic("kmalloc: memory corruption detected!\n");
        }
        if (i%10 == 0)
        {
            puts_color(" 10 x KMemory test passed\n", GREEN);
        }

        kfree(vm);
        kfree(vm2);
    }

    size = KB(3);

    vm1 = kmalloc(size);
    if (!vm1)
    {
        set_putchar_color(RED);
        printf("kmalloc: failed to allocate %z bytes\n", size);
        set_putchar_color(LIGHT_GREY);
        return;
    }

    memset(vm1, 'A', size);
    for (i = 0; i < size; i++)
    {
        if (((char*)vm1)[i] != 'A')
        {
            set_putchar_color(RED);
            printf("kmalloc: memory corruption detected!\n");
            set_putchar_color(LIGHT_GREY);
            kernel_panic("kmalloc: memory corruption detected!\n");
        }
    }
    kfree(vm1);
}

static void test_vmalloc()
{
    size_t size;
    void* vm;
    void* vm1;
    void* vm2;
    int i;

    for (i = 0; i < 500; i++)
    {
        size = KB(1);
        vm = vmalloc(size, false);
        if (!vm)
        {
            set_putchar_color(RED);
            printf("vmalloc: failed to allocate %z bytes\n", size);
            set_putchar_color(LIGHT_GREY);
            return;
        }
        vm2 = vmalloc(size, false);
        if (!vm2)
        {
            set_putchar_color(RED);
            printf("vmalloc: failed to allocate %z bytes\n", size);
            set_putchar_color(LIGHT_GREY);
            return;
        }
        memset(vm, 'A', size);
        memset(vm2, 'A', size);
        if (memcmp(vm, vm2, size) != 0)
        {
            set_putchar_color(RED);
            printf("vmalloc: memory corruption detected!\n");
            set_putchar_color(LIGHT_GREY);
            kernel_panic("vmalloc: memory corruption detected!\n");
        }
        if (i%10 == 0)
        {
            puts_color(" 10 x VMemory test passed\n", GREEN);
        }
        vfree(vm);
        vfree(vm2);
    }

    size = KB(2);
    vm1 = vmalloc(size, true);
    if (!vm1)
    {
        set_putchar_color(RED);
        printf("vmalloc: failed to allocate %z bytes\n", size);
        set_putchar_color(LIGHT_GREY);
        return;
    }

    memset(vm1, 'A', size);
    for (i = 0; i < size; i++)
    {
        if (((char*)vm1)[i] != 'A')
        {
            set_putchar_color(RED);
            printf("vmalloc: memory corruption detected!\n");
            set_putchar_color(LIGHT_GREY);
            kernel_panic("vmalloc: memory corruption detected!\n");
        }
    }
    
    vfree(vm1);
}

static void show_kernel_allocations()
{
    printf("Kernel Allocations (kmalloc):\n");
    block_header_t* current = free_list;
    while (current)
    {
        printf("  Block at %p: size=%z, free=%d\n", 
                (void*)current, current->size, current->free);
        current = current->next;
    }
}

static void show_user_allocations()
{
    printf("User Allocations (vmalloc):\n");
    vblock_header_t* current = vblock_list;
    while (current)
    {
        printf("  Block at %p: size=%z, free=%d\n",
                (void*)current, current->size, current->free);
        current = current->next;
    }
}

#include "../tasks/task.h"
void debug_task_stack(task_t *task)
{
    // uint32_t pd_index = (uintptr_t)task->stack >> 22;
    // uint32_t pt_index = ((uintptr_t)task->stack >> 12) & 0x3FF;

    // printf("Debugging task stack for PID %d:\n", task->pid);
    // printf("  Stack Base: %x\n", (uint32_t)task->stack);
    // printf("  Page Directory Entry: %x\n", page_directory[pd_index]);

    // if (page_directory[pd_index] & PAGE_PRESENT)
    // {
    //     page_table_t* table = (page_table_t*)(page_directory[pd_index] & ~0xFFF);
    //     printf("  Page Table Entry: %x\n", (*table)[pt_index]);
    // }
    // else
    // {
    //     printf("  PDE not present!\n");
    // }
}
