#ifndef MEMORY_H
#define MEMORY_H

#include "../utils/stdint.h"

#define PAGE_DIRECTORY_ENTRIES 1024
#define PAGE_TABLE_ENTRIES 1024
#define PAGE_SIZE 0x1000  /* 4 KB */
#define PAGE_PRESENT 0x1
#define PAGE_WRITE 0x2
#define PAGE_USER 0x4

// typedef struct {
//     uint32_t entries[PAGE_DIRECTORY_ENTRIES];
// } page_directory_t;

// typedef struct {
//     uint32_t entries[PAGE_TABLE_ENTRIES];
// } page_table_t;

void paging_init();

#endif // MEMORY_H