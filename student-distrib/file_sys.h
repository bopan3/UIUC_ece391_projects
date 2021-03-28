#ifndef _FILE_SYS_H
#define _FILE_SYS_H

#include "types.h"

// Starting address of file system
uint32_t file_sys_addr;

// Function declarations
void filesys_init();
int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry);
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry);

#endif
