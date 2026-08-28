#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#include "py/mpstate.h"
#include "py/lexer.h"
#include "py/parse.h"
#include "py/compile.h"
#include "py/runtime.h"
#include "py/gc.h"
#include "py/stackctrl.h"
#include "py/mphal.h"

#include "drivers/vga.h"
#include "drivers/keyboard.h"
#include "drivers/mem.h"
#include "initrd.h"

extern size_t strlen(const char* str);
extern int serial_has_data(void);
extern char serial_getc(void);
extern int keyboard_has_data(void);
extern unsigned char keyboard_getc(void);
extern uint32_t stack_top;

// Receive a single character from either the PS/2 keyboard or Serial COM1
int mp_hal_stdin_rx_chr(void) {
    while (1) {
        if (keyboard_has_data()) {
            return keyboard_getc();
        }
        if (serial_has_data()) {
            char c = serial_getc();
            if (c == '\r') return '\n';
            return c;
        }
        asm volatile("hlt");
    }
}

// Send a string to the VGA console
mp_uint_t mp_hal_stdout_tx_strn(const char *str, size_t len) {
    terminal_write(str, len);
    return len;
}

// Convert \n to \r\n for the terminal
void mp_hal_stdout_tx_strn_cooked(const char *str, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (str[i] == '\n') {
            terminal_putchar('\r');
        }
        terminal_putchar(str[i]);
    }
}

// Garbage collection: scan the C stack for live MicroPython object pointers
void gc_collect(void) {
    volatile uint32_t regs[4];
    asm volatile (
        "mov %%ebx, %0\n"
        "mov %%esi, %1\n"
        "mov %%edi, %2\n"
        "mov %%ebp, %3\n"
        : "=m"(regs[0]), "=m"(regs[1]), "=m"(regs[2]), "=m"(regs[3])
        :: "memory"
    );

    gc_collect_start();

    // Scan from current stack pointer up to stack_top
    void *sp;
    asm volatile ("mov %%esp, %0" : "=r"(sp));
    void *st = (void*)&stack_top;
    if ((uintptr_t)sp < (uintptr_t)st) {
        gc_collect_root((void**)sp, ((uintptr_t)st - (uintptr_t)sp) / sizeof(void*));
    }

    gc_collect_end();
}

// Required for MicroPython to function
void nlr_jump_fail(void *val) {
    (void)val;
    terminal_writestring("FATAL: MicroPython NLR jump failed\n");
    while (1);
}

#define HISTORY_SIZE 10
static char cmd_history[HISTORY_SIZE][256];
static int history_count = 0;
static int history_current = 0;

void nexus_readline(char *line, int max_len) {
    int i = 0;
    int esc_state = 0;
    int h_ptr = history_current;

    while (i < max_len - 1) {
        char c = (char)mp_hal_stdin_rx_chr();

        if (esc_state == 0) {
            if (c == '\x1b') {
                esc_state = 1;
            } else if (c == '\n' || c == '\r') {
                line[i] = '\0';
                terminal_putchar('\n');
                break;
            } else if ((c == '\b' || c == 127) && i > 0) {
                i--;
                terminal_putchar('\b');
            } else if (c >= 32 && c <= 126) {
                line[i++] = c;
                terminal_putchar(c);
            }
        } else if (esc_state == 1) {
            if (c == '[') esc_state = 2;
            else esc_state = 0;
        } else if (esc_state == 2) {
            if (c == 'A') { // Up Arrow
                if (history_count > 0) {
                    while (i > 0) { terminal_putchar('\b'); i--; }
                    h_ptr = (h_ptr - 1 + HISTORY_SIZE) % HISTORY_SIZE;
                    for (int j = 0; cmd_history[h_ptr][j] != '\0'; j++) {
                        line[i++] = cmd_history[h_ptr][j];
                        terminal_putchar(cmd_history[h_ptr][j]);
                    }
                }
            } else if (c == 'B') { // Down Arrow
                while (i > 0) { terminal_putchar('\b'); i--; }
                if (history_count > 0) {
                    h_ptr = (h_ptr + 1) % HISTORY_SIZE;
                    for (int j = 0; cmd_history[h_ptr][j] != '\0'; j++) {
                        line[i++] = cmd_history[h_ptr][j];
                        terminal_putchar(cmd_history[h_ptr][j]);
                    }
                }
            }
            esc_state = 0;
        }
    }

    // Add to history if not empty
    if (i > 0) {
        for (int j = 0; j < 256; j++) cmd_history[history_current][j] = line[j];
        history_current = (history_current + 1) % HISTORY_SIZE;
        if (history_count < HISTORY_SIZE) history_count++;
    }
}

// MicroPython input() hook implementation
int readline(vstr_t *vstr, const char *p) {
    if (p && p[0]) {
        mp_hal_stdout_tx_strn_cooked(p, strlen(p));
    }
    char line[256];
    nexus_readline(line, sizeof(line));
    vstr_add_str(vstr, line);
    return 0;
}

int mp_hal_readline(vstr_t *vstr, const char *p) {
    return readline(vstr, p);
}


// The main loop for the Python REPL in Nexus OS
void start_nexus_python(void) {
    mp_stack_ctrl_init();
    mp_stack_set_limit(50000); 

    // GC heap placed at 7MB — safely above the kernel heap (0x200000–0x600000)
    void* gc_heap = (void*)0x700000;
    gc_init(gc_heap, (uint8_t*)gc_heap + (512 * 1024)); // 512 KB GC heap

    mp_init();

    // Check for Nexus AI script in storage
    size_t script_size = 0;
    uint8_t* script_data = initrd_get_file("nexus_ai.py", &script_size);

    if (script_data) {
        terminal_writestring("\n Launching Nexus AI Persona...\n");
        nlr_buf_t nlr;
        if (nlr_push(&nlr) == 0) {
            char* script_buf = m_new(char, script_size + 2);
            memcpy(script_buf, script_data, script_size);
            script_buf[script_size] = '\n';
            script_buf[script_size + 1] = '\0';
            mp_lexer_t *lex = mp_lexer_new_from_str_len(MP_QSTR__lt_stdin_gt_, script_buf, script_size + 1, 0);
            mp_parse_compile_execute(lex, MP_PARSE_FILE_INPUT, mp_globals_get(), mp_locals_get());
            nlr_pop();
        } else {
            terminal_writestring(" AI Script Error:\n");
            mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
        }
    }

    terminal_writestring("\n--- Nexus Python v0.1.0 ---\n");
    terminal_writestring("Type 'help()' for more info.\n");

    char line[256];
    char exec_buf[260];
    while (1) {
        terminal_writestring(">>> ");
        nexus_readline(line, 256);

        if (line[0] != '\0') {
            // Append newline for parser
            int len = 0;
            while (line[len]) { exec_buf[len] = line[len]; len++; }
            exec_buf[len++] = '\n';
            exec_buf[len] = '\0';

            nlr_buf_t nlr;
            if (nlr_push(&nlr) == 0) {
                mp_lexer_t *lex = mp_lexer_new_from_str_len(MP_QSTR__lt_stdin_gt_, exec_buf, len, 0);
                mp_parse_compile_execute(lex, MP_PARSE_SINGLE_INPUT, mp_globals_get(), mp_locals_get());
                nlr_pop();
            } else {
                mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
            }
        }
    }


    mp_deinit();
}

