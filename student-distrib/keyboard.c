#include "keyboard.h"
#include "lib.h"
#include "i8259.h"
#include "terminal.h"

#define SCANCODE_SET_SIZE 58
#define EMP 0x0
#define IRQ_NUM_KEYBOARD 0x01
#define KEYBOARD_DATA_PORT 0x60

/* Special Keycodes */
#define BCKSPACE                0x08
#define ENTER                   0x0A

#define L_SHIFT_PRESS           0x2A
#define L_SHIFT_RELEASE         0xAA
#define R_SHIFT_PRESS           0x36
#define R_SHIFT_RELEASE         0xB6
#define CTRL_PRESS              0x1D
#define CTRL_RELEASE            0x9D
#define CAPS_PRESS              0x3A



/* KEY_FLAGS */
volatile uint8_t enter_flag = 0;
static uint8_t l_shift_flag = 0;
static uint8_t r_shift_flag = 0;
static uint8_t ctrl_flag = 0;
static uint8_t caps_flag = 0;

#define SHIFT_FLAG              (l_shift_flag | r_shift_flag)

/*
* The table used to map the scancode to ascii
* The table is adapt from https://wiki.osdev.org/Keyboard
*/
uint8_t scancode_to_ascii[SCANCODE_SET_SIZE][2] = {
    {EMP, EMP}, {EMP, EMP},     
    {'1', '!'}, {'2', '@'},
    {'3', '#'}, {'4', '$'},
    {'5', '%'}, {'6', '^'},
    {'7', '&'}, {'8', '*'},
    {'9', '('}, {'0', ')'},
    {'-', '_'}, {'=', '+'},
    {BCKSPACE, BCKSPACE}, {' ', ' '},    
    {'q', 'Q'}, {'w', 'W'},
    {'e', 'E'}, {'r', 'R'},
    {'t', 'T'}, {'y', 'Y'},
    {'u', 'U'}, {'i', 'I'},
    {'o', 'O'}, {'p', 'P'},
    {'[', '{'}, {']', '}'},
    {ENTER, ENTER}, {EMP, EMP},  // , Left Control
    {'a', 'A'}, {'s', 'S'},
    {'d', 'D'}, {'f', 'F'},
    {'g', 'G'}, {'h', 'H'},
    {'j', 'J'}, {'k', 'K'},
    {'l', 'L'}, {';', ':'},
    {'\'', '"'}, {'`', '~'},
    {EMP, EMP}, {'\\', '|'},    // Left Shift,
    {'z', 'Z'}, {'x', 'X'},
    {'c', 'C'}, {'v', 'V'},
    {'b', 'B'}, {'n', 'N'},
    {'m', 'M'}, {',', '<'},
    {'.', '>'}, {'/', '?'},
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
 *   SIDE EFFECTS:  convert the scancode (from port) to ascii, and print it onto the terminal
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

    if ((scan_code >= SCANCODE_SET_SIZE) || (scan_code < 0x02)){
        send_eoi(IRQ_NUM_KEYBOARD);
        sti();
        return;
    }
    if (scan_code < SCANCODE_SET_SIZE && scan_code >= 0){

        if (ctrl_flag & ('l' == scancode_to_ascii[scan_code][0])) {
            clear();
        } else {
            // Set ascii_code in respond to caps_flag and SHIFT_FLAG
            if (caps_flag | SHIFT_FLAG)
                ascii_code = scancode_to_ascii[scan_code][1];
            else
                ascii_code = scancode_to_ascii[scan_code][0];

            // Display to screen and feed to line_buffer
            putc(ascii_code);
            line_buf_in(ascii_code);
        }
        send_eoi(IRQ_NUM_KEYBOARD);
        sti();
    }
}

/*
 * spe_key_check
 *   DESCRIPTION: check for special keys pressed or not
 *   INPUTS: scan_code -- the incoming key code
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS:  convert the scancode (from port) to ascii, and print it onto the terminal
 */
int spe_key_check(uint8_t scan_code) {
    switch (scan_code) {
        case CAPS_PRESS:
            if (0 == caps_flag)
                caps_flag = 1;
            else
                caps_flag = 0;
            return 1;
        case ENTER:
            enter_flag = 1;
            return 1;
        case L_SHIFT_PRESS:
            l_shift_flag = 1;
            return 1;
        case R_SHIFT_PRESS:
            r_shift_flag = 1;
            return 1;
        case L_SHIFT_RELEASE:
            l_shift_flag = 0;
            return 1;
        case R_SHIFT_RELEASE:
            r_shift_flag = 0;
            return 1;
        case CTRL_PRESS:
            ctrl_flag = 1;
            return 1;
        case CTRL_RELEASE:
            ctrl_flag = 0;
            return 1;
        default:
            return 0;
    }
}

