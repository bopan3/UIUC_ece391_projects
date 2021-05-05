#ifndef _mouse_h_
#define _mouse_h_

#include "types.h"
#include "lib.h"
#include "ModeX.h"

#define MOUSE_IRQ_NUM       12
#define MOUSE_PORT_NUM      0x64
#define KETBOARD_PORT_NUM   0x60

#define VERY_LONG_TIME      100000
#define WAIT_OUT_MASK       2
#define WAIT_IN_MASK        1

#define SIGN_MASK           0xFFFFFF00

#define X_BLOCK_NUM         26
#define Y_BLOCK_NUM         15

#define MOUSE_SPEED_FACTOR  3

/* Number of icon in screen */
#define NUM_ICON 5

/* Use a number to indicate each file */
#define DOT             0
#define SIGTEST         1
#define SHELL           2
#define GREP            3
#define SYSERR          4      
#define RTC             5
#define FISH            6
#define COUNTER         7
#define PINGPONG        8
#define CAT             9
#define FRAME0_TXT      10
#define VERY_LONG_TXT   11
#define LS              12
#define TESTPRINT       13
#define CREATED_TXT     14
#define FRAME1_TXT      15
#define HELLO           16

/* Structure of mouse's data package */
typedef struct mouse_package {
    uint8_t left_btn    : 1;
    uint8_t right_btn   : 1;
    uint8_t mid_btn     : 1;
    uint8_t always_1    : 1;
    uint8_t x_sign      : 1;
    uint8_t y_sign      : 1;
    uint8_t x_overflow  : 1;
    uint8_t y_overflow  : 1;
} mouse_package;

/* Function define */
void mouse_init();
void wait_in();
void wait_out();
uint8_t read_port();
void write_port(uint8_t data);
void mouse_irq_handler();
void screen_layout_init();

#endif
