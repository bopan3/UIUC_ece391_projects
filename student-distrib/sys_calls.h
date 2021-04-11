#ifndef _SYS_CALLS_H
#define _SYS_CALLS_H

#include "types.h"

/* Some parameters */
#define N_FILES 8
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


/* File descriptor */
typedef struct file_des_t {
    file_ops*   fop_t;
    uint32_t    idx_inode;
    uint32_t    file_type;
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
