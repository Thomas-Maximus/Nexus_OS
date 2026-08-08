#ifndef MPHALPORT_H
#define MPHALPORT_H

#include <stdint.h>
#include <stddef.h>

// MicroPython HAL definitions for Nexus OS
static inline void mp_hal_set_interrupt_char(int c) {
    (void)c;
}

static inline uint32_t mp_hal_ticks_ms(void) {
    // We'll need a real timer driver for this, for now return mock
    return 0;
}

static inline void mp_hal_delay_ms(uint32_t ms) {
    // Minimal busy wait or mock
    for (volatile uint32_t i = 0; i < ms * 10000; i++) ;
}

#endif
