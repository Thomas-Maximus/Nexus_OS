#ifndef IDT_H
#define IDT_H

#include <stdint.h>

struct idt_entry {
    uint16_t base_lo;
    uint16_t sel;        /* Kernel segment selector */
    uint8_t  always0;    /* This must always be zero! */
    uint8_t  flags;      /* Flags (Type/Attributes) */
    uint16_t base_hi;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

void idt_install();
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);

#endif
