#include "mouse.h"
#include "i8259.h"
#include "ModeX.h"
#include "blocks.h"
#include "desktop.h"

/* Global variables */
int32_t mouse_x_move;
int32_t mouse_y_move;
int32_t mouse_x_coor;       // Set to the middle of GUI
int32_t mouse_y_coor;       // Set to the middle of GUI
int32_t mouse_key_left;     // 1 means pressed, 0 means not
int32_t mouse_key_right;    // 1 means pressed, 0 means not
int32_t mouse_key_mid;      // 1 means pressed, 0 means not

extern game_info_t game_info;

uint8_t icon_location[X_BLOCK_NUM][Y_BLOCK_NUM];
int center_blk_idx[NUM_ICON][2] = {
    {2, 2},
    {2, 7},
    {2, 12}
};
int center_blk_fnum[NUM_ICON] = {
    FISH,
    COUNTER,
    PINGPONG
};

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
    mouse_x_coor = SCROLL_X_DIM / 4;
    mouse_y_coor = SCROLL_Y_DIM / 4;


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
    outb(60, KETBOARD_PORT_NUM);

    /* Set i8259 */
    enable_irq(MOUSE_IRQ_NUM);

    screen_layout_init();
}

/* screen_layout_init
 *  Description: initialize the screen layout matrix and other parameters
 *  Input: none
 *  Output: none
 *  Return: none
 *  Side Effect: none
 */
void screen_layout_init() {
    int x, y, i;
    int file_num;

    for (x = 0; x < X_BLOCK_NUM; x++) {
        for (y = 0; y < Y_BLOCK_NUM; y++) {
            icon_location[x][y] = 0;
        }
    }

    /* Set matrix value for each icon */
    for (i = 0; i < NUM_ICON; i++) {
        file_num = center_blk_fnum[i];
        x = center_blk_idx[i][0];
        y = center_blk_idx[i][1];

        icon_location[x-1][y] = file_num;
        icon_location[x-1][y-1] = file_num;
        icon_location[x-1][y+1] = file_num;
        icon_location[x][y] = file_num;
        icon_location[x][y-1] = file_num;
        icon_location[x][y+1] = file_num;
        icon_location[x+1][y] = file_num;
        icon_location[x+1][y-1] = file_num;
        icon_location[x+1][y+1] = file_num;
    }


}


/* mouse_irq_handler
 *  Description: read input data from mouse
 *  Input: none
 *  Output: none
 *  Return: none
 *  Side Effect: write data to globle variables 
 */
void mouse_irq_handler() {
    uint8_t temp;
    mouse_package mouse_in;
    unsigned char restore_block[12*12];

    send_eoi(MOUSE_IRQ_NUM);
    // sti();
    // printf("[Test] is_ModX: %d\n", game_info.is_ModX);

    /* If not in GUI, do nothing */
    if (!game_info.is_ModX)
        return;

    /* Read data */
    temp = read_port();
    mouse_in.left_btn = temp & 0x01;
    mouse_in.right_btn = (temp & 0x02) >> 1;
    mouse_in.mid_btn = (temp & 0x04) >> 2;  
    mouse_in.always_1 = (temp & 0x08) >> 3; 
    mouse_in.x_sign = (temp & 0x10) >> 4;   
    mouse_in.y_sign = (temp & 0x20) >> 5;   
    mouse_in.x_overflow = (temp & 0x40) >> 6;
    mouse_in.y_overflow = (temp & 0x80) >> 7;

    /* Sanity check */
    if ((!mouse_in.always_1) || mouse_in.x_overflow || mouse_in.y_overflow)
        return;
    
    mouse_x_move = read_port();
    mouse_y_move = read_port();

    /* Sign extention */
    if (mouse_in.x_sign)
        mouse_x_move |= SIGN_MASK;
    if (mouse_in.y_sign)
        mouse_y_move |= SIGN_MASK;
    
    
    /* Update other parameters */
    mouse_x_coor += mouse_x_move / MOUSE_SPEED_FACTOR;
    mouse_y_coor -= mouse_y_move / MOUSE_SPEED_FACTOR;
    if (mouse_x_coor < 0)
        mouse_x_coor = 0;
    if (mouse_y_coor < 0)
        mouse_y_coor = 0;
    if (mouse_x_coor >= SCROLL_X_DIM - 10)
        mouse_x_coor = SCROLL_X_DIM - 11;
    if (mouse_y_coor >= SCROLL_Y_DIM -10)
        mouse_y_coor = SCROLL_Y_DIM - 11;

    /* Update press state */
    mouse_key_left = mouse_in.left_btn;
    mouse_key_right = mouse_in.right_btn;
    mouse_key_mid = mouse_in.mid_btn;

    

    // printf("[Test] (left, right, mid): (%d, %d, %d)\n", mouse_key_left, mouse_key_right, mouse_key_mid);

    cli();
    if (mouse_key_left || mouse_key_right || mouse_key_mid) {
        draw_full_block_with_mask(mouse_x_coor, mouse_y_coor, get_block_img(MOUSE_CURSOR), get_block_img(MOUSE_CURSOR_MASK_SOLID), restore_block);
        show_screen();
        restore_full_block_with_mask(mouse_x_coor, mouse_y_coor, get_block_img(MOUSE_CURSOR), get_block_img(MOUSE_CURSOR_MASK_SOLID), restore_block);
    } else {
        draw_fruit_text_with_mask(mouse_x_coor, mouse_y_coor, get_block_img(MOUSE_CURSOR_MASK_TRANS), restore_block);
        show_screen();
        restore_fruit_text_with_mask(mouse_x_coor, mouse_y_coor, get_block_img(MOUSE_CURSOR_MASK_TRANS), restore_block);
    }
    sti();

    /* Determine location of mouse w.r.t icon */
    int blk_y = mouse_x_coor / 12;
    int blk_x = mouse_y_coor / 12;
    int offset[4][4] = {{0,0}, {0,1}, {1,0}, {1,1}};
    int i;
    int f_num;

    for (i = 0; i < 4; i++) {
        if (blk_x + offset[i][0] < X_BLOCK_NUM && blk_y + offset[i][1] < Y_BLOCK_NUM) {
            f_num = icon_location[blk_x + offset[i][0]][blk_y + offset[i][1]];
        } else {
            f_num = 0;
        }

        if (f_num != 0) {
            
            break;
        }
    }
    
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

