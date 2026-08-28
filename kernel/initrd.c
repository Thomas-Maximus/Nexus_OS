#include "initrd.h"
#include "drivers/vga.h"

extern int strncmp(const char *s1, const char *s2, size_t n);

static uint32_t n_files = 0;
static uint8_t* initrd_base = 0;

void initrd_init(uint32_t location) {
    initrd_base = (uint8_t*)location;
    n_files = *(uint32_t*)location;
}

uint8_t* initrd_get_file(const char* name, size_t* size) {
    if (!initrd_base || n_files == 0 || !name) {
        return NULL;
    }
    // Layout (matches mkinitrd.py):
    //   [4 bytes: n_files] [n_files * 36-byte headers] [file data...]
    //   Each header: [32 bytes: name (null-padded)] [4 bytes: size]
    uint8_t* header_ptr = initrd_base + 4;
    uint8_t* data_ptr   = initrd_base + 4 + (n_files * 36);

    
    for (uint32_t i = 0; i < n_files; i++) {
        uint32_t fsize = *(uint32_t*)(header_ptr + 32);
        
        if (strncmp((char*)header_ptr, name, 32) == 0) {
            *size = fsize;
            return data_ptr;
        }
        
        header_ptr += 36;
        data_ptr += fsize;
    }
    
    return NULL;
}
