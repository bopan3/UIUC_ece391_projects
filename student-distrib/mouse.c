#include "mouse.h"
#include "i8259.h"

/* Global variables */
int32_t mouse_x_move;
int32_t mouse_y_move;
int32_t mouse_x_coor;       // Set to the middle of GUI
int32_t mouse_y_coor;       // Set to the middle of GUI
int32_t mouse_key_left;     // 1 means pressed, 0 means not
int32_t mouse_key_right;    // 1 means pressed, 0 means not
int32_t mouse_key_mid;      // 1 means pressed, 0 means not

/* mouse_init
 *  Description: initialize the mouse device
 *  Input: none
 *  Output: none
 *  Return: none
 *  Side Effect: initialize the mouse device
 */

void mouse_init() {
    uint8_t status;

    /* Initialize variables */
    mouse_x_move = 0;
    mouse_y_move = 0;
    mouse_key_left = 0;
    mouse_key_right = 0;
    mouse_key_mid = 0;

    /* Enbale auxiliary input of the PS2 keyboard controller */
    wait_out();
    outb(0xA8, MOUSE_PORT_NUM);

    /* Get compaq status byte */
    wait_out();
    outb(0x20, MOUSE_PORT_NUM);

    /* Read the status byte */
    wait_in();
    status = inb(KETBOARD_PORT_NUM);
    status |= 2;        // set bit number 1 (IRQ 12)
    status &= 0xDF;     // clear bit number 5 (disable mouse clock)

    /* Send command byte 0x60 ("Set Compaq Status") to port 0x64 
       modified Status byte to port 0x60 */
    wait_out();
    outb(KETBOARD_PORT_NUM, MOUSE_PORT_NUM);
    wait_out();
    outb(status, KETBOARD_PORT_NUM);

    /* Set defaults */
    write_port(0xF6);
    read_port();

    /* Enable packet streaming */
    write_port(0xF4);
    read_port();

    /* Set sample rate */
    write_port(0xF3);
    read_port();
    wait_out();
    outb(100, KETBOARD_PORT_NUM);

    /* Set i8259 */
    enable_irq(MOUSE_IRQ_NUM);

}


/* mouse_irq_handler
 *  Description: read input data from mouse
 *  Input: none
 *  Output: none
 *  Return: none
 *  Side Effect: write data to globle variables 
 */
void mouse_irq_handler() {
    mouse_package mouse_in[1];

    sti();
    send_eoi(MOUSE_IRQ_NUM);

    /* Read data */
    mouse_in[0] = read_port();

    /* Sanity check */
    if ((!mouse_in->always_1) || mouse_in->x_overflow || mouse_in->y_overflow)
        return;
    
    mouse_x_move = read_port();
    mouse_y_move = read_port();

    /* Sign extention */
    if (mouse_in->x_sign) {
        mouse_x_move |= SIGN_MASK;
    if (mouse_in->y_sign) {
        mouse_y_move |= SIGN_MASK;
    
    /* Update other parameters */
    mouse_x_coor += mouse_x_move;
    mouse_y_coor += mouse_y_move;
    if (mouse_x_coor < 0)
        mouse_x_coor = 0;
    if (mouse_y_coor < 0)
        mouse_y_coor = 0;
    if (mouse_x_coor >= /* TODO */)
        mouse_x_coor = /* TODO */ - 1;
    if (mouse_y_coor >= /* TODO */)
        mouse_y_coor = /* TODO */ - 1;

    /* Update press state */

}


/* wait_in
 *  Description: waiting to read bytes from port 0x60 or 0x64
 *  Input: none
 *  Output: none
 *  Return: none
 *  Side Effect: none 
 */
void wait_in() {
    int32_t wait_time = VERY_LONG_TIME;
    while (wait_time--) {
        if (!(inb(MOUSE_PORT_NUM) & WAIT_IN_MASK));
            break;
    }
}


/* wait_out
 *  Description: waiting to send bytes to port 0x60 or 0x64
 *  Input: none
 *  Output: none
 *  Return: none
 *  Side Effect: none 
 */
void wait_out() {
    int32_t wait_time = VERY_LONG_TIME;
    while (wait_time--) {
        if (!(inb(MOUSE_PORT_NUM) & WAIT_OUT_MASK));
            break;
    }
}


/* read_port
 *  Description: read port 0x60
 *  Input: none
 *  Output: none
 *  Return: read byte from 0x60
 *  Side Effect: no
 */
uint8_t read_port() {
    wait_in();
    return inb(KETBOARD_PORT_NUM);
}


/* write_port
 *  Description: write port 0x60
 *  Input: data to be write
 *  Output: none
 *  Return: none
 *  Side Effect: write byte to 0x60
 */
void write_port(uint8_t data) {
    /* Send 0xD4 preceding to acutal command */
    wait_out();
    outb(0xD4, MOUSE_PORT_NUM);
    /* Send actual command */
    wait_out();
    outb(data, KETBOARD_PORT_NUM);
}

