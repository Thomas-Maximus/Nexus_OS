#include "mem.h"

static uint32_t heap_start = 0;
static uint32_t heap_end = 0;
static uint32_t current_addr = 0;

void mem_initialize(uint32_t start_addr, size_t size) {
    heap_start = start_addr;
    heap_end = start_addr + size;
    current_addr = start_addr;
}

void* kmalloc(size_t size) {
    if (current_addr + size > heap_end) {
        return NULL; // Out of memory
    }
    
    void* ptr = (void*)current_addr;
    current_addr += size;
    
    /* Align to 4 bytes */
    if (current_addr % 4 != 0) {
        current_addr += (4 - (current_addr % 4));
    }
    
    return ptr;
}

void kfree(void* ptr) {
    /* Bump allocator doesn't support free */
    (void)ptr;
}
