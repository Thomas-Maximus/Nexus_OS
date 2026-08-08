#ifndef INITRD_H
#define INITRD_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    char name[32];
    uint32_t size;
    uint8_t* data;
} initrd_file_t;

void initrd_init(uint32_t location);
uint8_t* initrd_get_file(const char* name, size_t* size);

#endif
