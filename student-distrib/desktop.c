#include "ModeX.h"
#include "text.h"
#include "paging.h"
#include "i8259.h"
#include "timer.h"
#include "keyboard.h"
#include "desktop.h"
#include "blocks.h"

// variables borrowed from mazegame.c
typedef struct {
    /* dynamic values within a level -- you may want to add more... */
    unsigned int map_x, map_y;   /* current upper left display pixel */
} game_info_t;

static game_info_t game_info;

/*
 * The maze array contains a one byte bit vector (maze_bit_t) for each
 * location in a maze.  The left and right boundaries of the maze are unified,
 * i.e., column 0 also forms the right boundary via wraparound.  Drawings
 * for each block in the maze are chosen based on a five-point stencil
 * that includes north, east, south, and west neighbor blocks.  For
 * simplicity, the maze array is extended with additional rows on the top
 * and bottom to avoid boundary conditions.
 *
 * Under these assumptions, the upper left boundary of the maze is at (0,1),
 * and the lower right boundary is at (2 X_DIM, 2 Y_DIM + 1).  The stencil
 * calculation for the lower right boundary includes the point below it,
 * which is (2 X_DIM, 2 Y_DIM + 2).  As the width of the maze is 2 X_DIM,
 * the index of this point is 2 X_DIM + (2 Y_DIM + 2) * 2 X_DIM, or
 * 2 X_DIM (2 Y_DIM + 3), and the space allocated is one larger than this
 * maximum index value.
 */
static unsigned char maze[2 * MAZE_MAX_X_DIM * (2 * MAZE_MAX_Y_DIM + 3) + 1];
static int maze_x_dim;          /* horizontal dimension of maze */
static int maze_y_dim;          /* vertical dimension of maze   */
/* 
 * maze array index calculation macro; maze dimensions are valid only
 * after a call to make_maze
 */
#define MAZE_INDEX(a,b) ((a) + ((b) + 1) * maze_x_dim * 2)


/*
*	desktop_open
*	Description: provide desktop the access to a file
*	inputs:	filename -- name of the file (not used)
*	outputs: nothing
*	effects: none
*/
int32_t desktop_open(const uint8_t* filename) {
    int32_t i;
    switch_to_modeX();
    //stop schedule
    disable_irq(PIT_IRQ);        

    /* Initialize dynamic values. */
    game_info.map_x = game_info.map_y = SHOW_MIN;
    // make the maze (desktop)
    make_desktop(MAZE_MIN_X_DIM, MAZE_MIN_Y_DIM);

    /* Set logical view and draw initial screen. */
    set_view_window(game_info.map_x, game_info.map_y);
    for (i = 0; i < SCROLL_Y_DIM; i++)
        draw_horiz_line (i);

    refresh_bar(NULL,NULL,NULL);
    show_screen();
    return 0;
}

/*
*	desktop_close
*	Description: close the specific file descriptor
*	inputs:	filename -- name of the file descriptor
*	outputs: nothing
*	effects: none
*/
int32_t desktop_close(int32_t fd) {
    set_text_mode_3(0);
    //reopen schedule
    enable_irq(PIT_IRQ);
    return 0;
}

/*
 *   desktop_read -- system call
 *   DESCRIPTION: Read data from mouse click
 *   INPUTS: fd -- not used
 *           buf -- the line input buffer
 *           nbytes -- the max size of the input buffer
 *   OUTPUTS: none
 *   RETURN VALUE:  -- number of bytes read from the buffer
 *   SIDE EFFECTS: Send end-of-interrupt signal for the specified IRQ
 */
// int32_t desktop_read(int32_t fd, void* buf, int32_t nbytes) {
//     // int i;                          // Loop index

//     // // check NULL pointer and wrong nbytes
//     // if (buf == NULL)
//     //     return -1;

//     // // While enter not pressed, wait for enter
//     // while (OFF == enter_flag || desktop_tick != desktop_display) {}
//     // cli();

//     // // Define a temp buffer for data transfer
//     // int8_t * temp_buf = (int8_t *)buf;
//     // // Copy the buffer content
//     // for (i = 0; (i < nbytes-1) && (i < LINE_BUF_SIZE); i++) {
//     //     temp_buf[i] = tm_array[desktop_display].kb_buf[i];
//     //     if ('\n' == tm_array[desktop_display].kb_buf[i]){
//     //         i++;
//     //         break;
//     //     }
//     // }

//     // // Clear the buffer
//     // line_buf_clear();
//     // enter_flag = OFF;

//     // sti();
//     // return i;
// }

// /*
//  *   desktop_write -- system call
//  *   DESCRIPTION: write the data in given buffer to the desktop console
//  *   INPUTS: fd -- not used
//  *           buf -- the buffer to be output
//  *           nbytes -- the max size of the output buffer
//  *   OUTPUTS: none
//  *   RETURN VALUE:  -- number of bytes read from the buffer
//  *   SIDE EFFECTS: Send end-of-interrupt signal for the specified IRQ
//  */
// int32_t desktop_write(int32_t fd, const void* buf, int32_t nbytes) {
//     // int i;                          // Loop index
//     // uint8_t curr;

//     // // check NULL pointer and wrong nbytes
//     // if (buf == NULL)
//     //     return -1;

//     // cli();
//     // for(i = 0; i < nbytes; ++i) {
//     //     curr = ((char*) buf)[i];
//     //     if(curr != '\0')            // Skip null
//     //         putc(curr);             // Print other characters
//     // }
//     // sti();
//     return 0;
// }






/* 
 * make_desktop
 *   DESCRIPTION: Create a desktop of specified dimensions.  
 *   INPUTS: (x_dim,y_dim) -- size of desktop
 *   OUTPUTS: none
 *   RETURN VALUE: 0 on success, -1 on failure (if requested maze size
 *              exceeds limits set by defined values, with minimum
 *              (MAZE_MIN_X_DIM,MAZE_MIN_Y_DIM) and maximum
 *              (MAZE_MAX_X_DIM,MAZE_MAX_Y_DIM))
 *   SIDE EFFECTS: leaves MAZE_REACH markers on marked portion of maze
 */
int make_desktop(int x_dim, int y_dim) {
 
    //int x, y;

    /* Check the requested size, and save in local state if it is valid. */
    if (x_dim < MAZE_MIN_X_DIM || x_dim > MAZE_MAX_X_DIM ||
        y_dim < MAZE_MIN_Y_DIM || y_dim > MAZE_MAX_Y_DIM)
        return -1;
    maze_x_dim = x_dim;
    maze_y_dim = y_dim;

    /* Fill the maze with walls. */
    memset(maze, BLOCK_FRUIT_1, sizeof (maze));
    maze[MAZE_INDEX(10, 10)] = MOUSE_CURSOR;

    //  /* Remove all walls! */
    // for (x = 1; x < 2 * maze_x_dim; x++) {
    //     for (y = 1; y < 2 * maze_y_dim; y++) {
    //         maze[MAZE_INDEX(x, y)] = MAZE_NONE;
    //     }
    // }

    return 0;
}


/* 
 * find_block
 *   DESCRIPTION: Find the appropriate image to be used for a given maze
 *                lattice point.
 *   INPUTS: (x,y) -- the maze lattice point
 *   OUTPUTS: none
 *   RETURN VALUE: a pointer to an image of a BLOCK_X_DIM x BLOCK_Y_DIM
 *                 block of data with one byte per pixel laid out as a
 *                 C array of dimension [BLOCK_Y_DIM][BLOCK_X_DIM]
 *   SIDE EFFECTS: none
 */
static unsigned char* find_block(int x, int y) { return (unsigned char*) blocks[maze[MAZE_INDEX(x, y)]]; }

/* 
 * fill_horiz_buffer
 *   DESCRIPTION: Given the (x,y) map pixel coordinate of the leftmost 
 *                pixel of a line to be drawn on the screen, this routine 
 *                produces an image of the line.  Each pixel on the line
 *                is represented as a single byte in the image.
 *   INPUTS: (x,y) -- leftmost pixel of line to be drawn 
 *   OUTPUTS: buf -- buffer holding image data for the line
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void fill_horiz_buffer(int x, int y, unsigned char buf[SCROLL_X_DIM]) {
    int map_x, map_y;     /* maze lattice point of the first block on line */
    int sub_x, sub_y;     /* sub-block address                             */
    int idx;              /* loop index over pixels in the line            */ 
    unsigned char* block; /* pointer to current maze block image           */

    /* Find the maze lattice point and the pixel address within that block. */
    map_x = x / BLOCK_X_DIM;
    map_y = y / BLOCK_Y_DIM;
    sub_x = x - map_x * BLOCK_X_DIM;
    sub_y = y - map_y * BLOCK_Y_DIM;

    /* Loop over pixels in line. */
    for (idx = 0; idx < SCROLL_X_DIM; ) {

        /* Find address of block to be drawn. */
        block = find_block(map_x++, map_y) + sub_y * BLOCK_X_DIM + sub_x;

        /* Write block colors from one line into buffer. */
        for (; idx < SCROLL_X_DIM && sub_x < BLOCK_X_DIM; idx++, sub_x++)
            buf[idx] = *block++;

        /* 
         * All subsequent blocks are copied starting from the left side 
         * of the block. 
         */
        sub_x = 0;
    }
}

/* 
 * fill_vert_buffer
 *   DESCRIPTION: Given the (x,y) map pixel coordinate of the top pixel of 
 *                a vertical line to be drawn on the screen, this routine 
 *                produces an image of the line.  Each pixel on the line
 *                is represented as a single byte in the image.
 *   INPUTS: (x,y) -- top pixel of line to be drawn 
 *   OUTPUTS: buf -- buffer holding image data for the line
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void fill_vert_buffer(int x, int y, unsigned char buf[SCROLL_Y_DIM]) {
    int map_x, map_y;     /* maze lattice point of the first block on line */
    int sub_x, sub_y;     /* sub-block address                             */
    int idx;              /* loop index over pixels in the line            */ 
    unsigned char* block; /* pointer to current maze block image           */

    /* Find the maze lattice point and the pixel address within that block. */
    map_x = x / BLOCK_X_DIM;
    map_y = y / BLOCK_Y_DIM;
    sub_x = x - map_x * BLOCK_X_DIM;
    sub_y = y - map_y * BLOCK_Y_DIM;

    /* Loop over pixels in line. */
    for (idx = 0; idx < SCROLL_Y_DIM; ) {

        /* Find address of block to be drawn. */
        block = find_block(map_x, map_y++) + sub_y * BLOCK_X_DIM + sub_x;

        /* Write block colors from one line into buffer. */
        for (; idx < SCROLL_Y_DIM && sub_y < BLOCK_Y_DIM; 
                idx++, sub_y++, block += BLOCK_X_DIM)
            buf[idx] = *block;

        /* 
         * All subsequent blocks are copied starting from the top
         * of the block. 
         */
        sub_y = 0;
    }

    return;
}
