#include "keyboard.h"
#include "lib.h"
#include "i8259.h"
#define SCANCODE_SET_SIZE 58
#define BCKSPACE 0x0
#define ENTER 0x0
#define EMP 0x0
#define IRQ_NUM_KEYBOARD 0x01
#define KEYBOARD_DATA_PORT 0x60
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
void keyboard_init(){
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
void keyboard_handler(){
    cli();
    uint8_t scan_code = inb(KEYBOARD_DATA_PORT);
    uint8_t ascii_code;
    if (scan_code < SCANCODE_SET_SIZE && scan_code >= 0){
        ascii_code = scancode_to_ascii[scan_code][0];
        putc(ascii_code);
    }
    send_eoi(IRQ_NUM_KEYBOARD);
    sti();
}

