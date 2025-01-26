#ifndef MEMORY_H
#define MEMORY_H

#include "../utils/stdint.h"

#define PAGE_DIRECTORY_ENTRIES  1024
#define PAGE_TABLE_ENTRIES      1024
#define PAGE_SIZE               0x1000  /* 4 KB */
#define PAGE_PRESENT            0x1
#define PAGE_WRITE              0x2
#define PAGE_RW                 0x2
#define PAGE_USER               0x4

void paging_init();

void* kbrk(void* addr);
void kfree(void* ptr);
void* kmalloc(size_t size);
size_t ksize(void* ptr);
void heap_init();

void dump_page_directory();
void debug_page_mapping(uint32_t address);

void* vmalloc(size_t size, int is_user);
void vfree(void* ptr);
size_t vsize(void* ptr);

#endif // MEMORY_H
