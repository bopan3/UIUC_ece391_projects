#ifndef _FILE_SYS_H
#define _FILE_SYS_H

#include "types.h"
#include "sys_calls.h"

// Starting address of file system
uint32_t file_sys_addr;

// Function declarations
void filesys_init();
int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry);
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry);
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length);

int32_t file_open(const uint8_t* filename);
int32_t file_read(int32_t fd, void* buf, int32_t nbytes);
int32_t file_write(int32_t fd, const void* buf, int32_t nbytes);
int32_t file_close(int32_t fd);

int32_t direct_open(const uint8_t* directname);
int32_t direct_read(int32_t fd, void* buf, int32_t nbytes);
int32_t direct_write(int32_t fd, const void* buf, int32_t nbytes);
int32_t direct_close(int32_t fd);

int32_t get_file_size(uint32_t inode);

#endif
