#ifndef MEM_H
#define MEM_H

#include <stddef.h>
#include <stdint.h>

void mem_initialize(uint32_t start_addr, size_t size);
void* kmalloc(size_t size);
void kfree(void* ptr);

#endif
