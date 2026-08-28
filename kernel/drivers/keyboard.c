#include "keyboard.h"
#include "idt.h"
#include "vga.h"
#include <stdint.h>
#include <stdbool.h>

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ( "inb %w1, %b0" : "=a"(ret) : "Nd"(port) : "memory");
    return ret;
}

static const unsigned char kbd_us[128] = {
    0,   27, '1', '2', '3', '4', '5', '6', '7', '8',	/* 0-9 */
  '9',  '0', '-', '=', '\b',	/* Backspace */
  '\t',			/* Tab */
  'q',  'w', 'e', 'r',	/* 16-19 */
  't',  'y', 'u', 'i', 'o', 'p', '[', ']', '\n',	/* Enter key */
    0,			/* 29   - Control */
  'a',  's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',	/* 30-39 */
 '\'',  '`',   0,		/* Left shift (42) */
 '\\',  'z', 'x', 'c', 'v', 'b', 'n',			/* 43-49 */
  'm',  ',', '.', '/',   0,				/* Right shift (54) */
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

static const unsigned char kbd_us_shift[128] = {
    0,   27, '!', '@', '#', '$', '%', '^', '&', '*',	/* 0-9 */
  '(',  ')', '_', '+', '\b',	/* Backspace */
  '\t',			/* Tab */
  'Q',  'W', 'E', 'R',	/* 16-19 */
  'T',  'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',	/* Enter key */
    0,			/* 29   - Control */
  'A',  'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',	/* 30-39 */
  '"',  '~',   0,		/* Left shift (42) */
  '|',  'Z', 'X', 'C', 'V', 'B', 'N',			/* 43-49 */
  'M',  '<', '>', '?',   0,				/* Right shift (54) */
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

#define KBD_BUFFER_SIZE 256
static unsigned char kbd_buffer[KBD_BUFFER_SIZE];
static volatile int kbd_buffer_head = 0;
static volatile int kbd_buffer_tail = 0;

static bool shift_pressed = false;
static bool caps_lock = false;
static bool ctrl_pressed = false;
static bool ext_scancode = false;

void kbd_buffer_put(unsigned char c) {
    int next = (kbd_buffer_head + 1) % KBD_BUFFER_SIZE;
    if (next != kbd_buffer_tail) {
        kbd_buffer[kbd_buffer_head] = c;
        kbd_buffer_head = next;
    }
}

static void kbd_buffer_puts(const char* s) {
    while (*s) {
        kbd_buffer_put((unsigned char)*s++);
    }
}

int keyboard_has_data(void) {
    return (kbd_buffer_head != kbd_buffer_tail);
}

unsigned char keyboard_getc(void) {
    while (!keyboard_has_data()) {
        asm volatile("hlt");
    }
    unsigned char c = kbd_buffer[kbd_buffer_tail];
    kbd_buffer_tail = (kbd_buffer_tail + 1) % KBD_BUFFER_SIZE;
    return c;
}

void keyboard_handler(struct regs *r) {
    (void)r;
    uint8_t scancode = inb(0x60);

    if (scancode == 0xE0) {
        ext_scancode = true;
        return;
    }

    if (ext_scancode) {
        ext_scancode = false;
        if (!(scancode & 0x80)) {
            switch (scancode) {
                case 0x48: kbd_buffer_puts("\x1b[A"); return; // Up Arrow
                case 0x50: kbd_buffer_puts("\x1b[B"); return; // Down Arrow
                case 0x4D: kbd_buffer_puts("\x1b[C"); return; // Right Arrow
                case 0x4B: kbd_buffer_puts("\x1b[D"); return; // Left Arrow
                case 0x47: kbd_buffer_puts("\x1b[H"); return; // Home
                case 0x4F: kbd_buffer_puts("\x1b[F"); return; // End
                case 0x53: kbd_buffer_put(127); return;       // Delete
                default: break;
            }
        }
        return;
    }

    // Key release (break code)
    if (scancode & 0x80) {
        uint8_t released = scancode & 0x7F;
        if (released == 0x2A || released == 0x36) {
            shift_pressed = false;
        } else if (released == 0x1D) {
            ctrl_pressed = false;
        }
        return;
    }

    // Key press (make code)
    if (scancode == 0x2A || scancode == 0x36) {
        shift_pressed = true;
        return;
    }
    if (scancode == 0x1D) {
        ctrl_pressed = true;
        return;
    }
    if (scancode == 0x3A) {
        caps_lock = !caps_lock;
        return;
    }

    // Keypad / non-extended arrow keys fallback
    if (scancode == 0x48) { kbd_buffer_puts("\x1b[A"); return; }
    if (scancode == 0x50) { kbd_buffer_puts("\x1b[B"); return; }
    if (scancode == 0x4D) { kbd_buffer_puts("\x1b[C"); return; }
    if (scancode == 0x4B) { kbd_buffer_puts("\x1b[D"); return; }

    if (scancode < 128) {
        unsigned char base_c = kbd_us[scancode];
        if (base_c == 0) return;

        if (ctrl_pressed) {
            if (base_c >= 'a' && base_c <= 'z') {
                kbd_buffer_put((unsigned char)(base_c - 'a' + 1));
                return;
            }
            if (base_c >= 'A' && base_c <= 'Z') {
                kbd_buffer_put((unsigned char)(base_c - 'A' + 1));
                return;
            }
        }

        unsigned char c;
        if (base_c >= 'a' && base_c <= 'z') {
            bool upper = shift_pressed ^ caps_lock;
            c = upper ? (base_c - 'a' + 'A') : base_c;
        } else {
            c = shift_pressed ? kbd_us_shift[scancode] : base_c;
        }

        if (c != 0) {
            kbd_buffer_put(c);
        }
    }
}

void keyboard_install(void) {
    irq_install_handler(1, keyboard_handler);
}

