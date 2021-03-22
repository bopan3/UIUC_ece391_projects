// terminal.h - Defines used in interactions with the terminal
//
// Created by 熊能 on 2021/3/21.
//

#ifndef MP3_TERMINAL_H
#define MP3_TERMINAL_H

#include "types.h"

#define BCKSPACE            0x08
#define LINE_BUF_SIZE       128

int32_t read(int32_t fd, void* buf, int32_t nbytes);

int32_t write(int32_t fd, const void* buf, int32_t nbytes);

void line_buf_in(char curr);

int line_input(char* buf);

#endif //MP3_TERMINAL_H
