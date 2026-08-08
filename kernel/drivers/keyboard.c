#include "keyboard.h"
#include "idt.h"
#include "vga.h"
#include <stdint.h>

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ( "inb %w1, %b0" : "=a"(ret) : "Nd"(port) : "memory");
    return ret;
}

unsigned char kbd_us[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8',	/* 9 */
  '9', '0', '-', '=', '\b',	/* Backspace */
  '\t',			/* Tab */
  'q', 'w', 'e', 'r',	/* 19 */
  't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',	/* Enter key */
    0,			/* 29   - Control */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',	/* 39 */
 '\'', '`',   0,		/* Left shift */
 '\\', 'z', 'x', 'c', 'v', 'b', 'n',			/* 49 */
  'm', ',', '.', '/',   0,				/* Right shift */
  '*',
    0,	/* Alt */
  ' ',	/* Space bar */
    0,	/* Caps lock */
    0,	/* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,	/* < ... F10 */
    0,	/* 69 - Num lock*/
    0,	/* Scroll Lock */
    0,	/* Home key */
    0,	/* Up Arrow */
    0,	/* Page Up */
  '-',
    0,	/* Left Arrow */
    0,
    0,	/* Right Arrow */
  '+',
    0,	/* 79 - End key*/
    0,	/* Down Arrow */
    0,	/* Page Down */
    0,	/* Insert Key */
    0,	/* Delete Key */
    0,   0,   0,
    0,	/* F11 Key */
    0,	/* F12 Key */
    0,	/* All other keys are undefined */
};

/* External registration logic from idt.c */
extern void irq_install_handler(int irq, void (*handler)(void *r));

#define KBD_BUFFER_SIZE 256
static unsigned char kbd_buffer[KBD_BUFFER_SIZE];
static int kbd_buffer_head = 0;
static int kbd_buffer_tail = 0;

void kbd_buffer_put(unsigned char c) {
    int next = (kbd_buffer_head + 1) % KBD_BUFFER_SIZE;
    if (next != kbd_buffer_tail) {
        kbd_buffer[kbd_buffer_head] = c;
        kbd_buffer_head = next;
    }
}

int keyboard_has_data() {
    return (kbd_buffer_head != kbd_buffer_tail);
}

unsigned char keyboard_getc() {
    while (!keyboard_has_data()) {
        /* Wait for character - in a real OS we'd yield or hlt */
        asm volatile("hlt");
    }
    unsigned char c = kbd_buffer[kbd_buffer_tail];
    kbd_buffer_tail = (kbd_buffer_tail + 1) % KBD_BUFFER_SIZE;
    return c;
}

void keyboard_handler(void *r) {
    (void)r;
    uint8_t scancode = inb(0x60);

    if (!(scancode & 0x80)) {
        unsigned char c = kbd_us[scancode];
        if (c != 0) {
            kbd_buffer_put(c);
        }
    }
}

void keyboard_install() {
    irq_install_handler(1, (void *)keyboard_handler);
}
