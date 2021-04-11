#ifndef _SYS_CALLS_H
#define _SYS_CALLS_H

#include "types.h"

/* Some parameters */
#define N_FILES     8
#define INI_FILES   2

#define INUSE   1
#define UNUSE   0

#define _8MB_ 0x800000
#define _8KB_ 0x2000
#define FAIL -1
#define SUCCESS 0
#define FILENAME_LEN 32
#define TERM_LEN 128
#define VALIDATION_READ_SIZE 40
#define MAX_PROC 6
#define INVALID_NODE -1
#define USER_START_SIZE 4
#define USER_ESP 0x8000000 + 0x400000 - 1 /* 128 MB for start user + 4 MB for stack size - 1 entry */
/* File operation tables */
// typedef struct file_ops_t {
//     /* TODO */
// } file_ops_t;

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
    uint32_t    file_pos;   // position where last read ends
    uint32_t    flags;     // flages that indicate file's state
} file_des_t;

/* PCB struct */
typedef struct pcb {
    file_des_t  file_array[N_FILES];
    uint8_t args[TERM_LEN];
    uint32_t pid;
    uint32_t prev_pid;

    // uint32_t user_esp;
    uint32_t user_eip;
    uint32_t kernel_esp;
    uint32_t kernel_eip;
} pcb;

fop_t rtc_fop_t;
fop_t dir_fop_t;
fop_t reg_fop_t;
fop_t stdi_fop_t;
fop_t stdo_fop_t;

/* end a process, returning the wanted value to its parent process */
//int32_t halt(uint8_t status);

/* initiate and start a new program, guide the processor to the new program */
//int32_t execute(const uint8_t* command);

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

#define _ASM_switch_(_0_SS, EBP, _0_CS, EIP)    \
do{                             \
    asm volatile ("             \n\
        pushl   %%eax           \n\
        pushl   %%ebx           \n\
        pushfl                  \n\
        pushl   %%ecx           \n\
        pushl   %%edx           \n\
        iret                    \n\
    "                           \
    :   /* no outputs */        \
    : "a"(_0_SS), "b"(ESP), "c"(_0_CS), "d"(EIP)    \
    :                                               \
    );                                              \
}while(0);

#endif
