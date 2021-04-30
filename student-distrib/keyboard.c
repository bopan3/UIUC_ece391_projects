#include "keyboard.h"
#include "lib.h"
#include "i8259.h"
#include "terminal.h"
#include "scheduler.h"

#define SCANCODE_SET_SIZE 58
#define EMP 0x0
#define IRQ_NUM_KEYBOARD 0x01
#define KEYBOARD_DATA_PORT 0x60

#define ON      1
#define OFF     0
#define LOWER   0
#define HIGHER  1
#define SPECIAL 1
#define COMMON  0

/* Special Keycodes */
#define BCKSPACE                0x08
#define ENTER                   0x0A

#define L_SHIFT_PRESS           0x2A
#define L_SHIFT_RELEASE         0xAA
#define R_SHIFT_PRESS           0x36
#define R_SHIFT_RELEASE         0xB6
#define CTRL_PRESS              0x1D
#define CTRL_RELEASE            0x9D
#define ALT_PRESS               0x38
#define ALT_RELEASE             0xB8
#define CAPS_PRESS              0x3A

#define F1                      0x3B
#define F2                      0x3C
#define F3                      0x3D

/* KEY_FLAGS */
static uint8_t l_shift_flag = OFF;
static uint8_t r_shift_flag = OFF;
static uint8_t ctrl_flag = OFF;
static uint8_t caps_flag = OFF;
static uint8_t alt_flag = OFF;

#define SHIFT_FLAG              (l_shift_flag | r_shift_flag)

/* Multi-Terminals */
extern int32_t terminal_tick;
extern int32_t terminal_display;
extern terminal_t tm_array[];

/*
* The table used to map the scancode to ascii
* The table is adapt from https://wiki.osdev.org/Keyboard
*/
uint8_t scancode_to_ascii[SCANCODE_SET_SIZE][2] = {     // two ascii char for each entry
    {EMP, EMP}, {EMP, EMP},     
    {'1', '!'}, {'2', '@'},
    {'3', '#'}, {'4', '$'},
    {'5', '%'}, {'6', '^'},
    {'7', '&'}, {'8', '*'},
    {'9', '('}, {'0', ')'},     // A
    {'-', '_'}, {'=', '+'},
    {BCKSPACE, BCKSPACE}, {' ', ' '},
    {'q', 'Q'}, {'w', 'W'},      // 10, 11
    {'e', 'E'}, {'r', 'R'},
    {'t', 'T'}, {'y', 'Y'},
    {'u', 'U'}, {'i', 'I'},
    {'o', 'O'}, {'p', 'P'},
    {'[', '{'}, {']', '}'},      // 1A 1B
    {ENTER, ENTER}, {EMP, EMP},  // , Left Control
    {'a', 'A'}, {'s', 'S'},
    {'d', 'D'}, {'f', 'F'},     // 20, 21
    {'g', 'G'}, {'h', 'H'},
    {'j', 'J'}, {'k', 'K'},
    {'l', 'L'}, {';', ':'},     // 26, 27
    {'\'', '"'}, {'`', '~'},    // 28, 29
    {EMP, EMP}, {'\\', '|'},    // Left Shift, 2A, 2B
    {'z', 'Z'}, {'x', 'X'},
    {'c', 'C'}, {'v', 'V'},
    {'b', 'B'}, {'n', 'N'},     // 30, 31
    {'m', 'M'}, {',', '<'},     // 32, 33
    {'.', '>'}, {'/', '?'},     // 34, 35
    {EMP, EMP}, {EMP, EMP},     // Right Shift,
    {EMP, EMP}, {' ', ' '},    
};

/* 
 * keyboard_init
 *   DESCRIPTION: initialize the keyboard
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS:  initialize the keyboard
 */
void keyboard_init() {
    enable_irq(IRQ_NUM_KEYBOARD);
}

/* 
 * keyboard_handler
 *   DESCRIPTION: IRQ handler for keyboard
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS:  convert the scancode (from port) to ascii, and pass it to terminal
 */
void keyboard_handler() {
    cli();

    uint8_t scan_code = inb(KEYBOARD_DATA_PORT);            // Store scan code from keyboard
    uint8_t ascii_code;                                     // Store the corresponding ascii_code

    // Check for special keys
    if (spe_key_check(scan_code)) {
        send_eoi(IRQ_NUM_KEYBOARD);
        sti();
        return;
    }

    /* Handle the terminal switch function */
    if (alt_flag) {
        // printf("Scan Code:%x\n", scan_code);
        switch (scan_code) {
            case F1:
                switch_visible_terminal(0);
                break;
            case F2:
                switch_visible_terminal(1);
                break;
            case F3:
                switch_visible_terminal(2);
                break;
            default:
                break;
        }
        send_eoi(IRQ_NUM_KEYBOARD);
        sti();
        return;
    } 

    // make sure inside legit range
    if ((scan_code >= SCANCODE_SET_SIZE) || (scan_code < 0x02)){    // < 0x02 since the first two are empty
        send_eoi(IRQ_NUM_KEYBOARD);
        sti();
        return;
    }

    if (scan_code < SCANCODE_SET_SIZE){
        if (ctrl_flag) {
            switch ((scancode_to_ascii[scan_code][LOWER])) {
                case 'l':
                    clear();
                    break;
                /* ============== only for local test ============== */
                case '1':
                    switch_visible_terminal(0);
                    break;
                case '2':
                    switch_visible_terminal(1);
                    break;
                case '3':
                    switch_visible_terminal(2);
                    break;
                /*==============  only for local test ============== */
                default:
                    break;
            }
        } 
        // else if (alt_flag) {
        //     printf("Scan Code:%x\n", scan_code);
        //     switch (scan_code) {
                
        //         case F1:
        //             switch_visible_terminal(0);
        //             break;
        //         case F2:
        //             switch_visible_terminal(1);
        //             break;
        //         case F3:
        //             switch_visible_terminal(2);
        //             break;
        //         default:
        //             break;
        //     }
        // } 
        else {
            // Set ascii_code in respond to caps_flag and SHIFT_FLAG
            // Spacial case for numbers and -,= do not change when only caps_flag
            // <= 0x0D for 1-9 and -,= , 0x1A for [, 0x1B for ], 0x27 for ;, 0x28 for ', 0x29 for `, 0x2B for \, 0x33 for ,, 0x34 for ., 0x35 for /
            if ((scan_code <= 0x0D) | (0x1A == scan_code) | (0x1B == scan_code) | (0x27 == scan_code) | (0x28 == scan_code) | (0x29 == scan_code) | (0x2B == scan_code) | (0x33 == scan_code) | (0x34 == scan_code) | (0x35 == scan_code)) {
                if (SHIFT_FLAG)
                    ascii_code = scancode_to_ascii[scan_code][HIGHER];
                else
                    ascii_code = scancode_to_ascii[scan_code][LOWER];
            } else {
                if ((caps_flag | SHIFT_FLAG) & (caps_flag != SHIFT_FLAG))
                    ascii_code = scancode_to_ascii[scan_code][HIGHER];
                else
                    ascii_code = scancode_to_ascii[scan_code][LOWER];
            }

            // Display to screen and feed to line_buffer
//            putc(ascii_code);
            line_buf_in(ascii_code);
        }
    } 
    send_eoi(IRQ_NUM_KEYBOARD);
    sti();
    return;
}

/*
 * spe_key_check
 *   DESCRIPTION: check for special keys pressed or not
 *   INPUTS: scan_code -- the incoming key code
 *   OUTPUTS: none
 *   RETURN VALUE: 1 if it is a special key, 0 if not
 *   SIDE EFFECTS:  convert the scancode (from port) to ascii, and print it onto the terminal
 */
int spe_key_check(uint8_t scan_code) {
    switch (scan_code) {
        case CAPS_PRESS:
            if (OFF == caps_flag)
                caps_flag = ON;
            else
                caps_flag = OFF;
            return SPECIAL;       // return 1 to shown special key found
        case L_SHIFT_PRESS:
            l_shift_flag = ON;
            return SPECIAL;
        case R_SHIFT_PRESS:
            r_shift_flag = ON;
            return SPECIAL;
        case L_SHIFT_RELEASE:
            l_shift_flag = OFF;
            return SPECIAL;
        case R_SHIFT_RELEASE:
            r_shift_flag = OFF;
            return SPECIAL;
        case CTRL_PRESS:
            ctrl_flag = ON;
            return SPECIAL;
        case CTRL_RELEASE:
            ctrl_flag = OFF;
            return SPECIAL;
        case ALT_PRESS:
            alt_flag = ON;
            return SPECIAL;
        case ALT_RELEASE:
            alt_flag = OFF;
            return SPECIAL;
        default:
            return COMMON;
    }
}

