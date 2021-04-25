// terminal.h - Defines used in interactions with the terminal
//
// Created by 熊能 on 2021/3/21.
//

#ifndef MP3_TERMINAL_H
#define MP3_TERMINAL_H

#include "types.h"

#define BCKSPACE            0x08
#define LINE_BUF_SIZE       128

extern int32_t terminal_open(const uint8_t* filename);

extern int32_t terminal_close(int32_t fd);

extern int32_t terminal_read(int32_t fd, void* buf, int32_t nbytes);

extern int32_t terminal_write(int32_t fd, const void* buf, int32_t nbytes);

void line_buf_in(char curr);

void line_buf_clear(void);

void put_dis_ter(char curr);

#endif //MP3_TERMINAL_H
