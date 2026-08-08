/* --- Nexus OS Kernel --- */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "drivers/vga.h"
#include "drivers/gdt.h"
#include "drivers/mem.h"
#include "drivers/idt.h"
#include "drivers/keyboard.h"
#include "initrd.h"

/* --- Serial Port (COM1) for Debugging --- */
#define COM1_PORT 0x3F8

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ( "outb %b0, %w1" : : "a"(val), "Nd"(port) : "memory");
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ( "inb %w1, %b0" : "=a"(ret) : "Nd"(port) : "memory");
    return ret;
}

int init_serial() {
   outb(COM1_PORT + 1, 0x00);    // Disable all interrupts
   outb(COM1_PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
   outb(COM1_PORT + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
   outb(COM1_PORT + 1, 0x00);    //                  (hi byte)
   outb(COM1_PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
   outb(COM1_PORT + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
   outb(COM1_PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
   return 0;
}

int is_transmit_empty() {
   return inb(COM1_PORT + 5) & 0x20;
}

int serial_has_data() {
   return inb(COM1_PORT + 5) & 0x01;
}

char serial_getc() {
    while (serial_has_data() == 0);
    return inb(COM1_PORT);
}

void write_serial(char a) {
   while (is_transmit_empty() == 0);
   outb(COM1_PORT, a);
}

void write_serial_string(const char* s) {
    for (size_t i = 0; s[i] != '\0'; i++) {
        write_serial(s[i]);
    }
}

/* --- Multiboot Structure --- */
struct multiboot_info {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
};

struct multiboot_module {
    uint32_t mod_start;
    uint32_t mod_end;
    uint32_t string;
    uint32_t reserved;
};

void kernel_main(uint32_t magic, struct multiboot_info* mbi) {
	terminal_initialize();
    init_serial();

    write_serial_string("SERIAL: Nexus OS Kernel Booting...\n");
    if (magic != 0x2BADB002) {
        write_serial_string("SERIAL: Error - Invalid Multiboot Magic\n");
    }

    gdt_install();
    write_serial_string("SERIAL: GDT Installed\n");
    mem_initialize(0x200000, 4 * 1024 * 1024); // 4MB heap starting at 2MB
    write_serial_string("SERIAL: Memory Initialized\n");

    /* Initialize RAMDisk if present */
    if (mbi->mods_count > 0) {
        struct multiboot_module* mod = (struct multiboot_module*)mbi->mods_addr;
        initrd_init(mod->mod_start);
        write_serial_string("SERIAL: InitRD Initialized\n");
    } else {
        write_serial_string("SERIAL: No InitRD found!\n");
    }

    idt_install();
    write_serial_string("SERIAL: IDT Installed\n");
    keyboard_install();
    write_serial_string("SERIAL: Keyboard Installed\n");

    /* Enable Interrupts */
    asm volatile("sti");
    write_serial_string("SERIAL: Interrupts Enabled\n");

	terminal_writestring(" Nexus OS Kernel v0.1.0\n");
	terminal_writestring(" ----------------------\n");
    write_serial_string("SERIAL: Terminal Initialized\n");
	terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    terminal_writestring(" Core initialized successfully.\n");
    terminal_writestring(" WSL Build System Active.\n");
    terminal_writestring("\n Starting Nexus Project Layer...\n");
    terminal_writestring(" Initializing Python Environment...\n");

    extern void start_nexus_python();
    start_nexus_python();

    write_serial_string("SERIAL: Core Initialized Successful\n");

    /* Loop forever */
    while(1);
}
