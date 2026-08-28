#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

void keyboard_install(void);
int keyboard_has_data(void);
unsigned char keyboard_getc(void);

#endif

