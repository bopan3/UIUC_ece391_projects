// terminal.c - Functions used in interactions with the terminal
//
// Created by 熊能 on 2021/3/21.
//

#include "terminal.h"
#include "lib.h"

volatile uint8_t enter_flag = 0;

static char line_buf[LINE_BUF_SIZE];    /* The line buffer */
static int num_char = 0;                /* Record current number of chars in line buffer */
static int enter_pressed = 0;           /* Record the state of whether enter is pressed */

/*
*	terminal_open
*	Description: provide terminal the access to a file
*	inputs:	filename -- name of the file
*	outputs: nothing
*	effects: none
*/
int32_t terminal_open(const uint8_t filename) {
    return 0;
}

/*
*	terminal_close
*	Description: close the specific file descriptor
*	inputs:	filename -- name of the file descriptor
*	outputs: nothing
*	effects: none
*/
int32_t terminal_close(int32_t fd) {
    return 0;
}

/*
 *   terminal_read -- system call
 *   DESCRIPTION: Read data from one line that has been terminated by pressing
 *                  Enter, or as much as fits in the buffer from one such line
 *                  include the line feed character
 *   INPUTS: fd -- not used
 *           buf -- the line input buffer
 *           nbytes -- the max size of the input buffer
 *   OUTPUTS: none
 *   RETURN VALUE:  -- number of bytes read from the buffer
 *   SIDE EFFECTS: Send end-of-interrupt signal for the specified IRQ
 */
int32_t terminal_read(int32_t fd, void* buf, int32_t nbytes) {
    int i;                          // Loop index

    // While enter not pressed, wait for enter
    while (0 == enter_flag) {}
    cli();

    int8_t * temp_buf = (int8_t *)buf;
    for (i = 0; (i < nbytes-1) && (i < LINE_BUF_SIZE); i++) {
        temp_buf[i] = line_buf[i];
    }

    line_buf_clear();
    enter_flag = 0;

    sti();
    return i;
}

/*
 *   terminal_write -- system call
 *   DESCRIPTION: write the data in given buffer to the terminal console
 *   INPUTS: fd -- not used
 *           buf -- the buffer to be output
 *           nbytes -- the max size of the output buffer
 *   OUTPUTS: none
 *   RETURN VALUE:  -- number of bytes read from the buffer
 *   SIDE EFFECTS: Send end-of-interrupt signal for the specified IRQ
 */
int32_t terminal_write(int32_t fd, const void* buf, int32_t nbytes) {
    int i;                          // Loop index
    uint8_t curr;
    cli();
    for(i = 0; i < nbytes; ++i) {
        curr = ((char*) buf)[i];
        if(curr != '\0')            // Skip null
            putc(curr);             // Print other characters
    }
    sti();
    return 0;
}

void line_buf_in(char curr) {
    // If the line buffer is already full, only change* when receiving line feed
    if (num_char >= LINE_BUF_SIZE) {
        if (('\n' == curr) | ('\r' == curr)) {
            line_buf[LINE_BUF_SIZE - 2] = '\n';
            enter_flag = 1;
            putc(curr);
        } else if (BCKSPACE == curr) {
            line_buf[LINE_BUF_SIZE - 1] = '\0';
            num_char = LINE_BUF_SIZE - 1;
            putc(curr);
        }
    } else {
        if (('\n' == curr) | ('\r' == curr)) {
            line_buf[num_char] = '\n';
            enter_flag = 1;
            putc(curr);
        } else if (BCKSPACE == curr) {
            if (num_char > 0 && num_char < LINE_BUF_SIZE) {
                line_buf[--num_char] = '\0';
                putc(curr);
            }
        } else {
            line_buf[num_char++] = curr;
            putc(curr);
        }
    }
}

void line_buf_clear() {
    int i;                      // loop index
    for (i = 0; i < LINE_BUF_SIZE ; ++i) {
        line_buf[i] = '\0';
    }
    num_char = 0;
}
