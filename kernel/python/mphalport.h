#ifndef MPHALPORT_H
#define MPHALPORT_H

#include <stdint.h>
#include <stddef.h>

extern uint32_t timer_get_ticks(void);

static inline void mp_hal_set_interrupt_char(int c) {
    (void)c;
}

static inline uint32_t mp_hal_ticks_ms(void) {
    return timer_get_ticks();
}

static inline void mp_hal_delay_ms(uint32_t ms) {
    uint32_t start = timer_get_ticks();
    while (timer_get_ticks() - start < ms) {
        asm volatile("hlt");
    }
}

#endif

