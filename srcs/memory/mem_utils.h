#ifndef MEM_UTILS_H
#define MEM_UTILS_H

void kfree(void* ptr);
void* kmalloc(size_t size);
size_t ksize(void* ptr);
void heap_init();

#endif // MEM_UTILS_H