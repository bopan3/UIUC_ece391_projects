#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "types.h"
#define IRQ_NUM_KEYBOARD 0x01

extern void keyboard_init();
extern void keyboard_handler();
int spe_key_check(uint8_t scan_code);
#endif

