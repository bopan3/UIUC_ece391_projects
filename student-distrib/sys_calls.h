#ifndef _SYS_CALLS_H
#define _SYS_CALLS_H

#include "types.h"

/* Some parameters */
#define N_FILES     8
#define INI_FILES   2

#define INUSE   1
#define UNUSE   0

#define FILE_RTC    0
#define FILE_DIREC  1
#define FILE_REG    2

/* File Operations Table Pointer */
typedef struct fop_t {
    int32_t (*read)(int32_t fd, void* buf, int32_t nbytes);
    int32_t (*write)(int32_t fd, const void* buf, int32_t nbytes);
    int32_t (*open)(const uint8_t* fname);
    int32_t (*close)(int32_t fd);
} fop_t;

/* File descriptor */
typedef struct file_des_t {
    fop_t*    file_ops_ptr;
    uint32_t    idx_inode;
    uint32_t    file_type;
    uint32_t    file_pos;   // position where last read ends
    uint32_t    flags;     // flages that indicate file's state
} file_des_t;

 /* PCB block */
 typedef struct pcb_block_t {
     file_des_t  file_array[N_FILES];
     /* TODO */
 } pcb_block_t;

fop_t rtc_fop_t;
fop_t dir_fop_t;
fop_t reg_fop_t;
fop_t stdi_fop_t;
fop_t stdo_fop_t;

/* end a process, returning the wanted value to its parent process */
int32_t halt(uint8_t status);

/* initiate and start a new program, guide the processor to the new program */
int32_t execute(const uint8_t* command);

/* open a file */
int32_t open(const uint8_t* fname);

/* close a file descriptor */
int32_t close(int32_t fd);

/* read data from keyboard, a file, device, or directory */
int32_t read(int32_t fd, void* buf, int32_t nbytes);

/* writes data to terminal or a device */
int32_t write(int32_t fd, const void* buf, int32_t nbytes);

/* bad calls */
int32_t badread(int32_t fd, void* buf, int32_t nbytes);
int32_t badwrite(int32_t fd, const void* buf, int32_t nbytes);

/* initialize the file operations table poinetr */
void fop_t_init();

#endif
