// terminal.c - Functions used in interactions with the terminal
//
// Created by 熊能 on 2021/3/21.
//

#include "terminal.h"
#include "lib.h"

static char line_buf[LINE_BUF_SIZE];    /* The line buffer */
static int num_char = 0;                /* Record current number of chars in line buffer */
static int enter_pressed = 0;           /* Record the state of whether enter is pressed */

void line_buf_in(char curr) {
    // If the line buffer is already full, only change* when receiving line feed
    if (num_char >= LINE_BUF_SIZE) {
        if (('\n' == curr) | ('\r' == curr)) {
            line_buf[LINE_BUF_SIZE-1] = '\n';
            enter_pressed = 1;
        } else if (BCKSPACE == curr) {
            line_buf[LINE_BUF_SIZE - 1] = ' ';
            num_char = LINE_BUF_SIZE - 1;
        } else
            num_char++;
    } else {
        if (('\n' == curr) | ('\r' == curr)) {
            line_buf[num_char] = '\n';
            enter_pressed = 1;
        } else if (BCKSPACE == curr) {
            if (num_char > 0)
                line_buf[--num_char] = ' ';
        }
        else
            line_buf[num_char++] = curr;
    }
}

/*
 *   read -- system call
 *   DESCRIPTION: Read data from keyboard, a file, a device (RTC) or directory
 *   INPUTS: irq_num -- the bit number of interruption
 *   OUTPUTS: none
 *   RETURN VALUE:  -- number of bytes read normally
 *                  -- 0 if the initial file position is at or beyond the end
 *                  of file (for normal files and the directory)
 *                  -- data from one line that has been terminated by pressing
 *                  Enter, or as much as fits in the buffer from one such line
 *                  include the line feed character (for keyboards)
 *                  -- repeatedly return 0 until subsequent reads from successive
 *                  directories reach the last (for directory)
 *                  -- Only return 0 when a interrupt occurs (for RTC)
 *   SIDE EFFECTS: Send end-of-interrupt signal for the specified IRQ
 */
int32_t read(int32_t fd, void* buf, int32_t nbytes) {

    return 0;
}
