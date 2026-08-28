#ifndef IDT_H
#define IDT_H

#include <stdint.h>
#include <stddef.h>

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

/* Stack frame passed to ISR and IRQ handlers */
struct regs {
    uint32_t ds;                                      /* Data segment selector */
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;  /* Pushed by pusha */
    uint32_t int_no, err_code;                        /* Interrupt number and error code */
    uint32_t eip, cs, eflags, useresp, ss;            /* Pushed by the processor automatically */
};

void idt_install(void);
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);
void irq_install_handler(int irq, void (*handler)(struct regs *r));
void irq_uninstall_handler(int irq);
void timer_install(void);
uint32_t timer_get_ticks(void);

#endif

