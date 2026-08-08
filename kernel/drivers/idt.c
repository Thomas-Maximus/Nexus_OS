#include "idt.h"
#include "vga.h"
#include <string.h>

struct idt_entry idt[256];
struct idt_ptr idtp;

/* Assembly handlers */
extern void idt_load();
extern void isr0();
extern void irq0();
extern void irq1();

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ( "outb %b0, %w1" : : "a"(val), "Nd"(port) : "memory");
}

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_lo = (base & 0xFFFF);
    idt[num].base_hi = (base >> 16) & 0xFFFF;
    idt[num].sel = sel;
    idt[num].always0 = 0;
    idt[num].flags = flags;
}

/* Remap PIC to avoid conflict with CPU exceptions */
void pic_remap() {
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20); // Master offset 32
    outb(0xA1, 0x28); // Slave offset 40
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    outb(0xA1, 0x01);
    outb(0x21, 0xFC); // Enable Timer (IRQ0) and Keyboard (IRQ1)
    outb(0xA1, 0xFF); // Mask all slave interrupts
}

void idt_install() {
    idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
    idtp.base = (uint32_t)&idt;

    /* Initialize with zeros */
    for(int i = 0; i < 256; i++) {
        idt_set_gate(i, 0, 0x08, 0x8E); // Kernel code selector 0x08
    }

    pic_remap();

    idt_set_gate(0, (uint32_t)isr0, 0x08, 0x8E);
    idt_set_gate(32, (uint32_t)irq0, 0x08, 0x8E);
    idt_set_gate(33, (uint32_t)irq1, 0x08, 0x8E);

    idt_load();
}

struct regs {
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
};

void isr_handler(struct regs *r) {
    terminal_writestring("Received Exception: ");
    /* Handle exceptions here */
    (void)r;
}

void (*irq_handlers[16])(struct regs *) = {0};

void irq_install_handler(int irq, void (*handler)(struct regs *r)) {
    irq_handlers[irq] = handler;
}

void irq_handler(struct regs *r) {
    void (*handler)(struct regs *r);

    handler = irq_handlers[r->int_no - 32];
    if (handler) {
        handler(r);
    }

    /* Send EOI to PIC */
    if (r->int_no >= 40) {
        outb(0xA0, 0x20);
    }
    outb(0x20, 0x20);
}
