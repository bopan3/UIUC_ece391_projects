#ifndef _SYS_CALLS_H
#define _SYS_CALLS_H

#include "types.h"

/* Some parameters */
#define N_FILES 8

/* File operation tables */
// typedef struct file_ops_t {
//     /* TODO */
// } file_ops_t;


/* File descriptor */
typedef struct file_des_t {
    // file_ops*   file_ops_ptr;
    uint32_t    idx_inode;
    uint32_t    file_pos;   // position where last read ends
    uint32_t    flages;     // flages that indicate file's state
} file_des_t;

// /* PCB block */
// typedef struct pcb_block_t {
//     file_des_t  file_array[N_FILES];
//     /* TODO */
// } pcb_block_t;


#endif
