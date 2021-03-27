/* types.h - Defines to use the familiar explicitly-sized types in this
 * OS (uint32_t, int8_t, etc.).  This is necessary because we don't want
 * to include <stdint.h> when building this OS
 * vim:ts=4 noexpandtab
 */

#ifndef _TYPES_H
#define _TYPES_H

#define NULL 0

#ifndef ASM

/* Types defined here just like in <stdint.h> */
typedef int int32_t;
typedef unsigned int uint32_t;

typedef short int16_t;
typedef unsigned short uint16_t;

typedef char int8_t;
typedef unsigned char uint8_t;

/* Types defined for file system */
typedef struct dentry_t {
    char        f_name[32];     // file name
    uint32_t    f_type;         // file type
    uint32_t    idx_inode;      // inode number
    uint8_t     reserved[24];   // resevered space
} dentry_t;

typedef struct boot_block_t {
    uint32_t    n_dentry;       // number of dir. entries
    uint32_t    n_inode;        // number of inodes
    uint32_t    n_data_block;   // number of data blocks
    uint8_t     reserved[52];   // reserved space
} boot_block_t;

typedef struct inode_block_t {
    uint32_t    length;             // length (in byte) of the file
    uint32_t    idx_block[1023];    // array of block indexes, 1023 =  4096 byte / 4 byte - 1
} inode_block_t;

typedef struct data_block_t {
    uint8_t     data[4096];     // 4KB byte addressable data
} data_block_t;

#endif /* ASM */

#endif /* _TYPES_H */
