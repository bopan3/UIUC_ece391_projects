// terminal.c - Functions used in interactions with the terminal
//
// Created by 熊能 on 2021/3/21.
//

#include "terminal.h"
#include "scheduler.h"
#include "lib.h"

#define ON          1
#define OFF         0

volatile uint8_t enter_flag = OFF;      /* Record the state of whether enter is pressed */
//static char line_buf[LINE_BUF_SIZE];    /* The line buffer */
//static int num_char = 0;                /* Record current number of chars in line buffer */

/* Multi-Terminals */
extern int32_t terminal_tick;
extern int32_t terminal_display;
extern terminal_t tm_array[];

#define NUM_CHAR    tm_array[terminal_tick].num_char
#define LINE_BUF    tm_array[terminal_tick].kb_buf

/*
*	terminal_open
*	Description: provide terminal the access to a file
*	inputs:	filename -- name of the file
*	outputs: nothing
*	effects: none
*/
int32_t terminal_open(const uint8_t* filename) {
    return -1;
}

/*
*	terminal_close
*	Description: close the specific file descriptor
*	inputs:	filename -- name of the file descriptor
*	outputs: nothing
*	effects: none
*/
int32_t terminal_close(int32_t fd) {
    return -1;
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

    // check NULL pointer and wrong nbytes
    if (buf == NULL)
        return -1;

    // While enter not pressed, wait for enter
    while (OFF == enter_flag) {}
    cli();

    // Define a temp buffer for data transfer
    int8_t * temp_buf = (int8_t *)buf;
    // Copy the buffer content
    for (i = 0; (i < nbytes-1) && (i < LINE_BUF_SIZE); i++) {
//        temp_buf[i] = line_buf[i];
        temp_buf[i] = LINE_BUF[i];
        if ('\n' == LINE_BUF[i]){
            i++;
            break;
        }
    }

    // Clear the buffer
    line_buf_clear();
    enter_flag = OFF;

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

    // check NULL pointer and wrong nbytes
    if (buf == NULL)
        return -1;

    cli();
    for(i = 0; i < nbytes; ++i) {
        curr = ((char*) buf)[i];
        if(curr != '\0')            // Skip null
            putc(curr);             // Print other characters
    }
    sti();
    return 0;
}

/*
 *   line_buf_in
 *   DESCRIPTION: receive input char from keyboard and store into line buffer
 *   INPUTS: curr -- the current input character
 *   OUTPUTS: display the character if it is successfully received
 *   RETURN VALUE: none
 *   SIDE EFFECTS: change the line input buffer and char index
 */
void line_buf_in(char curr) {
    // If the line buffer is already full, only change* when receiving line feed
    if (NUM_CHAR >= LINE_BUF_SIZE - 2) {        // minus 2 since the last two char of BUFFER must be '\n' and '\0'
        if (('\n' == curr) | ('\r' == curr)) {
            LINE_BUF[LINE_BUF_SIZE - 2] = '\n';
            enter_flag = ON;
            NUM_CHAR = 0;                       // reset buffer index to 0
            putc(curr);
        } else if (BCKSPACE == curr) {
            LINE_BUF[LINE_BUF_SIZE - 1] = '\0';
            NUM_CHAR--;
            putc(curr);
        }
    } else {
        if (('\n' == curr) | ('\r' == curr)) {
//            line_buf[NUM_CHAR] = '\n';
            LINE_BUF[NUM_CHAR] = '\n';
            enter_flag = ON;
            NUM_CHAR = 0;                       // reset buffer index to 0
            putc(curr);
        } else if (BCKSPACE == curr) {
            if (NUM_CHAR > 0 && NUM_CHAR < LINE_BUF_SIZE) { // If there are contents in buffer, delete the last one
                LINE_BUF[--NUM_CHAR] = '\0';
                putc(curr);
            }
        } else {
            LINE_BUF[NUM_CHAR++] = curr;
            putc(curr);
        }
    }
}

/*
 *   line_buf_clear
 *   DESCRIPTION: clear the line input buffer and reset index
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: clear the line input buffer and reset index
 */
void line_buf_clear() {
    int i;                      // loop index

    // fill with '\0'
    for (i = 0; i < LINE_BUF_SIZE ; ++i) {
//        line_buf[i] = '\0';
        LINE_BUF[i] = '\0';
    }
    NUM_CHAR = 0;
}
