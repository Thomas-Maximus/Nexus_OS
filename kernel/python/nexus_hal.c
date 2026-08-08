#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "py/mpstate.h"
#include "py/lexer.h"
#include "py/parse.h"
#include "py/compile.h"
#include "py/runtime.h"
#include "py/gc.h"
#include "py/stackctrl.h"

#include "drivers/vga.h"
#include "drivers/keyboard.h"
#include "drivers/mem.h"

extern size_t strlen(const char* str);

extern int serial_has_data();
extern char serial_getc();
extern int keyboard_has_data();

// Receive a single character from either the PS/2 keyboard or Serial COM1
int mp_hal_stdin_rx_chr(void) {
    while (1) {
        if (keyboard_has_data()) {
            return keyboard_getc();
        }
        if (serial_has_data()) {
            char c = serial_getc();
            // Handle Telnet/Terminal line endings if needed
            if (c == '\r') return '\n';
            return c;
        }
        // Yield the CPU while waiting
        asm volatile("hlt");
    }
}

// Send a string to the VGA console
void mp_hal_stdout_tx_strn(const char *str, size_t len) {
    terminal_write(str, len);
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

// Garbage collection trigger
void gc_collect(void) {
    // Basic stub - we need to scan stack/registers for pointers
    gc_collect_start();
    gc_collect_end();
}

// Required for MicroPython to function
void nlr_jump_fail(void *val) {
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
        char c = mp_hal_stdin_rx_chr();

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
                    // Clear current line
                    while (i > 0) { terminal_putchar('\b'); i--; }
                    
                    h_ptr = (h_ptr - 1 + HISTORY_SIZE) % HISTORY_SIZE;
                    for (int j = 0; cmd_history[h_ptr][j] != '\0'; j++) {
                        line[i++] = cmd_history[h_ptr][j];
                        terminal_putchar(cmd_history[h_ptr][j]);
                    }
                }
            } else if (c == 'B') { // Down Arrow
                // Simple version: clear line
                while (i > 0) { terminal_putchar('\b'); i--; }
                h_ptr = (h_ptr + 1) % HISTORY_SIZE;
                // ... logic to show newer if needed
            }
            esc_state = 0;
        }
    }

    // Add to history if not empty
    if (i > 0) {
        for(int j=0; j<256; j++) cmd_history[history_current][j] = line[j];
        history_current = (history_current + 1) % HISTORY_SIZE;
        if (history_count < HISTORY_SIZE) history_count++;
    }
}

#include "initrd.h"

// The main loop for the Python REPL in Nexus OS
void start_nexus_python() {
    mp_stack_set_limit(40000); 

    void* gc_heap = (void*)0x300000; // Fixed address for GC heap
    gc_init(gc_heap, (uint8_t*)gc_heap + (128 * 1024));

    mp_init();

    // Check for Nexus AI script in storage
    size_t script_size = 0;
    uint8_t* script_data = initrd_get_file("nexus_ai.py", &script_size);

    if (script_data) {
        terminal_writestring("\n Launching Nexus AI Persona...\n");
        nlr_buf_t nlr;
        if (nlr_push(&nlr) == 0) {
            mp_lexer_t *lex = mp_lexer_new_from_str_len(MP_QSTR__lt_stdin_gt_, (char*)script_data, script_size, 0);
            mp_parse_tree_t parse_tree = mp_parse(lex, MP_PARSE_FILE_INPUT);
            mp_obj_t module_fun = mp_compile(&parse_tree, lex->source_name, false);
            mp_call_function_0(module_fun);
            nlr_pop();
        } else {
            terminal_writestring(" AI Script Error.\n");
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
            while(line[len]) { exec_buf[len] = line[len]; len++; }
            exec_buf[len++] = '\n';
            exec_buf[len] = '\0';

            nlr_buf_t nlr;
            if (nlr_push(&nlr) == 0) {
                mp_lexer_t *lex = mp_lexer_new_from_str_len(MP_QSTR__lt_stdin_gt_, exec_buf, len, 0);
                mp_parse_tree_t parse_tree = mp_parse(lex, MP_PARSE_SINGLE_INPUT);
                mp_obj_t module_fun = mp_compile(&parse_tree, lex->source_name, false);
                mp_call_function_0(module_fun);
                nlr_pop();
            } else {
                terminal_writestring("Python Error.\n");
            }
        }
    }

    mp_deinit();
}
