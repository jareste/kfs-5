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
#define HEAP_START ((uintptr_t)&endkernel + 0x4000) // Start heap 4 KB after kernel
#define HEAP_SIZE_  0x100000  /* 1 MB heap size */
#define ALIGN_4K(x)  (((x) + 0xFFF) & ~0xFFF) /* 4 KB alignment */
#define ALIGN_8(x) (((x) + 0x7) & ~0x7) /* 8-byte alignment */
#define MB(x) (x * 1024 * 1024) /* Convert MB to bytes */
#define KB(x) (x * 1024) /* Convert KB to bytes */
#define MAX_HEAP_SIZE (100 * 1024 * 1024) /* Allow up to 100MB */

#define PAGE_PRESENT  0x1
#define PAGE_RW       0x2
#define PAGE_USER     0x4

/**
 * A simplistic “vmalloc region” from 0xC1000000 up to 0xC2000000
 * (16 MB) just as an example. Make sure it doesn't overlap your main
 * kernel heap or PDE[0] identity region.
 */
#define VMALLOC_START 0xC1000000
#define VMALLOC_END   0xC2000000  // 16 MB region

#define KERNEL_PDE_FLAGS  (PAGE_PRESENT | PAGE_RW)
#define KERNEL_PTE_FLAGS  (PAGE_PRESENT | PAGE_RW)

#define USER_PDE_FLAGS  (PAGE_PRESENT | PAGE_RW | PAGE_USER)
#define USER_PTE_FLAGS  (PAGE_PRESENT | PAGE_RW | PAGE_USER)

typedef struct block_header {
    size_t size;               /* Size of the block (excluding header) */
    struct block_header* next; /* Pointer to the next free block */
    int free;                  /* Is this block free? (1 for yes, 0 for no) */
} block_header_t;

/* Same than the upper one but for vmalloc */
typedef struct vblock_header {
    size_t size;
    struct vblock_header* next;
    int free;
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
void dump_page_directory();

/*############################################################################*/
/*                                                                            */
/*                           LOCALS                                           */
/*                                                                            */
/*############################################################################*/
static page_directory_t page_directory __attribute__((aligned(PAGE_SIZE)));

static block_header_t* free_list = NULL;
static void* heap_end;
static size_t HEAP_SIZE;

static vblock_header_t* vblock_list = NULL;
static uintptr_t vheap_end = VMALLOC_START;

static command_t commands[] = {
    {"f pfw", "Force a page fault by writing to an unmapped address", m_force_page_fault_write},
    {"f pfro", "Force a page fault by writing to a read-only page", m_force_page_fault_ro},
    {"dump pt", "Dump page table", dump_page_table},
    {"dump pd", "Dump page directory", dump_page_directory},
    {"tmem", "Test memory allocation", test_mem},
    {"theap", "Test dynamic heap growth", test_dynamic_heap_growth},
    {"vmalloc", "Test vmalloc", test_vmalloc},
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

void paging_init()
{
    pmm_init();

    memset(page_directory, 0, sizeof(page_directory));

    /* Map from 0 to 64 MB, each loop will map 4 KB 
     * Each PDE covers 4 MB, so we need up to PDE index = 15 to cover 64 MB.
     */

    const uint32_t IDENTITY_LIMIT = MB(64);// * 1024 * 1024; // 64 MB
    for (uintptr_t addr = 0; addr < IDENTITY_LIMIT; addr += 0x1000)
    {
        uint32_t pd_index = addr >> 22;        
        uint32_t pt_index = (addr >> 12) & 0x3FF; 

        if (!(page_directory[pd_index] & PAGE_PRESENT))
        {
            uint32_t pt_phys = allocate_frame();
            if (!pt_phys)
                kernel_panic("paging_init: out of frames for PDE\n");

            memset((void*)pt_phys, 0, PAGE_SIZE); 

            page_directory[pd_index] = (pt_phys & ~0xFFF) | (PAGE_PRESENT|PAGE_RW);
        }

        page_table_t* table = (page_table_t*)(page_directory[pd_index] & ~0xFFF);
        (*table)[pt_index] = (addr & ~0xFFF) | (PAGE_PRESENT|PAGE_RW);
    }

    asm volatile("mov %0, %%cr3" :: "r"(page_directory));

    /* Enable paging */
    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000; 
    asm volatile("mov %0, %%cr0" :: "r"(cr0));
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

static void map_page(uintptr_t virt_addr, uint32_t phys_addr, uint32_t flags)
{
    uint32_t pd_index = virt_addr >> 22;
    uint32_t pt_index = (virt_addr >> 12) & 0x3FF;

    if (!(page_directory[pd_index] & PAGE_PRESENT))
    {
        uint32_t pt_frame = allocate_frame();
        if (!pt_frame)
            kernel_panic("Out of frames for PDE!\n");

        /* PDE flags: take the top 20 bits for address plus flags 
         * Typically: PDE gets the same "user" bit if we want user access
         * If PDE is for kernel, do not set PAGE_USER
         */
        memset((void*)pt_frame, 0, PAGE_SIZE); // zero the frame
        uint32_t pde_flags = (pt_frame & ~0xFFF) | (flags & PAGE_USER) | PAGE_PRESENT | PAGE_RW;
        page_directory[pd_index] = pde_flags;
    }

    page_table_t* pt = (page_table_t*)(page_directory[pd_index] & ~0xFFF);

    (*pt)[pt_index] = (phys_addr & ~0xFFF) | (flags & 0xFFF);

    /* Flush TLB for this address */
    asm volatile("invlpg (%0)" :: "r"(virt_addr) : "memory");
}

static void map_page_kernel(uintptr_t virt, uint32_t phys)
{
    map_page(virt, phys, PAGE_PRESENT | PAGE_RW);
}

static void map_page_user(uintptr_t virt, uint32_t phys)
{
    map_page(virt, phys, PAGE_PRESENT | PAGE_RW | PAGE_USER);
}

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
        uint32_t page_directory_index = current_heap_end / (PAGE_SIZE * PAGE_TABLE_ENTRIES);
        uint32_t page_table_index = (current_heap_end % (PAGE_SIZE * PAGE_TABLE_ENTRIES)) / PAGE_SIZE;

        if (!(page_directory[page_directory_index] & 0x1))
        {
            uint32_t pt_phys = allocate_frame();
            if (!pt_phys) { kernel_panic("Failed to allocate frame for new page table"); }

            memset((void*)pt_phys, 0, PAGE_SIZE);
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

        memset((void*)pt_phys, 0, PAGE_SIZE);
        page_directory[pd_index] = pt_phys | make_pde_flags();
    }
    uint32_t pt_base = page_directory[pd_index] & ~0xFFF;
    return (page_table_t*)pt_base;
}

static void map_new_page(uintptr_t vaddr, bool is_user)
{
    uint32_t pde_flags = (PAGE_PRESENT | PAGE_RW);
    uint32_t pte_flags = (PAGE_PRESENT | PAGE_RW);

    if (is_user)
    {
        pde_flags |= PAGE_USER;
        pte_flags |= PAGE_USER;
    }

    uint32_t pd_index = vaddr >> 22;
    uint32_t pt_index = (vaddr >> 12) & 0x3FF;

    if (!(page_directory[pd_index] & PAGE_PRESENT))
    {
        uint32_t pt_phys = allocate_frame();
        if (!pt_phys)
        {
            kernel_panic("Out of frames for PDE!\n");
        }

        memset((void*)pt_phys, 0, PAGE_SIZE);
        page_directory[pd_index] = (pt_phys & ~0xFFF) | pde_flags;
    }

    page_table_t* pt = (page_table_t*)(page_directory[pd_index] & ~0xFFF);
    uint32_t frame_phys = allocate_frame();
    if (!frame_phys)
    {
        kernel_panic("Out of frames for PTE!\n");
    }

    memset((void*)frame_phys, 0, PAGE_SIZE);

    (*pt)[pt_index] = (frame_phys & ~0xFFF) | pte_flags;
    asm volatile("invlpg (%0)" :: "r"(vaddr) : "memory");
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

void* vbrk(void* addr, bool is_user)
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
        int i = 0;
        for (uintptr_t p = vheap_end; p < new_end; p += PAGE_SIZE)
        {
            // printf("vbrk: mapping new page %d\n", i++);
            // puts("vbrk: mapping new page\n");
            map_new_page(p, is_user);
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

void* vmalloc(size_t size, bool is_user)
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
    if (vbrk((void*)new_end, is_user) == (void*)-1)
    {
        puts_color("vmalloc: vbrk failed\n", RED);
        return NULL;
    }

    /* Insert a small header in front of the returned block. */
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
    uintptr_t end = start + sizeof(vblock_header_t) + size;
    start &= ~0xFFF;
    end = (end + 0xFFF) & ~0xFFF;

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
            // PDE present
            // Extract PDE flags
            uint32_t pde_flags = page_directory[i] & 0xFFF;     // low 12 bits
            uint32_t pde_base  = page_directory[i] & 0xFFFFF000; // top 20 bits

            printf("PDE[%d]: base=0x%x, flags=", i, pde_base);
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

/* A big test. */
static void test_dynamic_heap_growth()
{
    printf("Initial heap_end: %p\n", heap_end);

    void* block1 = kmalloc(64);
    printf("Allocated block1: %p\n", block1);

    size_t malloc_size = MB(75);
    void* large_block  = kmalloc(malloc_size);
    if (large_block)
    {
        printf("Allocated large_block: %p\n", large_block);
        printf("Block size: %z (hex=0x%x)\n", ksize(large_block), ksize(large_block));

        /* For demonstration, don't actually write 99 MB or you'll be slow. 
           But if you want, do:
             memset(large_block, 'A', malloc_size);
        */
    }
    else
    {
        puts_color("WARNING: Failed to allocate large block\n", RED);
    }

    printf("New heap_end: %p\n", heap_end);

    kfree(block1);
    kfree(large_block);

    printf("Heap freed.\n");
}

static void test_mem()
{
    void* block1 = kmalloc(64);
    printf("Allocated block1: %p (size=%z)\n", block1, ksize(block1));

    /* Fill it with data */
    memset(block1, 'A', 64);

    /* Free it */
    kfree(block1);

    /* Another test: 2 MB */
    void* block2 = kmalloc(2 * 1024 * 1024);
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

    printf("\nDebug mapping for address 0x%x\n", address);
    printf(" PDE index: %d, PDE entry: 0x%x\n", 
            pd_index, page_directory[pd_index]);

    if (page_directory[pd_index] & PAGE_PRESENT)
    {
        page_table_t* pt = (page_table_t*)(page_directory[pd_index] & ~0xFFF);
        printf("  PTE index: %d, PTE entry: 0x%x\n", 
                pt_index, (*pt)[pt_index]);
    }
    else
    {
        printf(" PDE not present.\n");
    }
}

static void test_vmalloc()
{
    size_t size = MB(10);
    void* vm1 = vmalloc(size, true);
    printf("Allocated vm1: %p\n", vm1);
    printf("Size of vm1: %z\n", vsize(vm1));
    printf("Size of vm1: %z\n", size);
    // vfree(vm1);
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
