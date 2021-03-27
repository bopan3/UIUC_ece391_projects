/*
 * Functions of file system
 */

#include "file_sys.h"
#include "lib.h"

/* Parameters of first entry in boot block */
static uint32_t n_dentry_b;
static uint32_t n_inode_b;
static uint32_t n_data_block_b;

/* Casting file_sys_addr to different type of pointers for convenience */
static struct dentry_t* p_dentry;              // pointer to dentry array (start from 0)
static struct inode_block_t* p_inode;          // pointer to inode block array
static struct data_block_t* p_data;            // pointer to data block array

/* Some parameters */
#define STR_LEN 32

/*-------------------- Helper functions --------------------*/ 

/* Note
 * 1. file_sys_addr indicate the starting address of file system 
 * 2. Cases that should return -1
 *      a. file name does not exists (read_dentry_by_name)
 *      b. invalde file index (read_dentry_by_index)
 *      c. inode number out of range (read_data)
 *      d.
 *      e.
 *      f.
 *      g.
 */ 

/* 
 * filesys_init
 *   DESCRIPTION: initialize pointers and parameters using file_sys_addr
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: pointers initialized
 */
void filesys_init() {
    n_dentry_b = ((boot_block_t*)file_sys_addr)->n_dentry;
    n_inode_b = ((boot_block_t*)file_sys_addr)->n_inode;
    n_data_block_b = ((boot_block_t*)file_sys_addr)->n_data_block;
    p_dentry = ((dentry_t*)file_sys_addr) + 1;        // skip the firt segment of boot block
    p_inode = ((inode_block_t*)file_sys_addr) + 1;    // skip the boot block 
    p_data = ((data_block_t*)file_sys_addr) + n_inode_b;      // skip the boot block and inode blocks
}

/* 
 * read_dentry_by_name
 *   DESCRIPTION: find the dir entry that has the given name
 *   INPUTS: fname - string of file name
 *           dentry - structure to pass output
 *   OUTPUTS: dentry structure
 *   RETURN VALUE: 0 if success, -1 if anything bad happened
 *   SIDE EFFECTS: none
 */
int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry) {

    int i, j;               // Loop counter
    uint8_t* fname_d;       // store name in dentry temporally
    int find_flage = 0;
    int length = strlen((int8_t*)fname);    // without '\0'

    // Check the length of input string
    if (length > STR_LEN) {
        return -1;
    }

    // Go through all dentries
    for (i = 0; i < n_dentry_b; i++) {

        fname_d = (uint8_t*)(p_dentry[i].f_name);
        find_flage = 1;

        // compare the name until a different characters or number 32 is reached
        for (j = 0; j <  length; j++) {
            if (fname[j] != fname_d[j]) {
                find_flage = 0;
                break;
            }
        }
        // After end, check if fname_d still have character
        if ((length < STR_LEN) && (fname_d[j] != '\0')) {
            find_flage = 0;
        }

        // Check flage
        if (find_flage == 1) {
            strncpy((int8_t*)(dentry->f_name), (int8_t*)(p_dentry[i].f_name), length);
            if (length < STR_LEN)
                dentry->f_name[length] = '\0';
            dentry->f_type = p_dentry[i].f_type;
            dentry->idx_inode = p_dentry[i].idx_inode; 
            return 0;
        }
    }

    // Not found, return -1
    return -1;
}

/* 
 * <name>
 *   DESCRIPTION: 
 *   INPUTS: 
 *   OUTPUTS: 
 *   RETURN VALUE: 0 if success, -1 if anything bad happened
 *   SIDE EFFECTS: none
 */
// int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry) {
//     return 0;
// }

/* 
 * <name>
 *   DESCRIPTION: 
 *   INPUTS: 
 *   OUTPUTS: 
 *   RETURN VALUE: 0 if success, -1 if anything bad happened
 *   SIDE EFFECTS: none
 */
// int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length) {
//     return 0;
// }

/*-------------------- Wrapper functions --------------------*/ 

// /* 
//  * <name>
//  *   DESCRIPTION: 
//  *   INPUTS: 
//  *   OUTPUTS: 
//  *   RETURN VALUE: 0 if success, -1 if anything bad happened
//  *   SIDE EFFECTS: none
//  */
// int32_t file_open() {
//     return 0;
// }

// /* 
//  * <name>
//  *   DESCRIPTION: 
//  *   INPUTS: 
//  *   OUTPUTS: 
//  *   RETURN VALUE: 0 if success, -1 if anything bad happened
//  *   SIDE EFFECTS: none
//  */
// int32_t file_read() {
//     return 0;
// }

// /* 
//  * <name>
//  *   DESCRIPTION: 
//  *   INPUTS: 
//  *   OUTPUTS: 
//  *   RETURN VALUE: 0 if success, -1 if anything bad happened
//  *   SIDE EFFECTS: none
//  */
// int32_t file_write() {
//     return 0;
// }

// /* 
//  * <name>
//  *   DESCRIPTION: 
//  *   INPUTS: 
//  *   OUTPUTS: 
//  *   RETURN VALUE: 0 if success, -1 if anything bad happened
//  *   SIDE EFFECTS: none
//  */
// int32_t file_close() {
//     return 0;
// }

// /* 
//  * <name>
//  *   DESCRIPTION: 
//  *   INPUTS: 
//  *   OUTPUTS: 
//  *   RETURN VALUE: 0 if success, -1 if anything bad happened
//  *   SIDE EFFECTS: none
//  */
// int32_t directory_open() {
//     return 0;
// }

// /* 
//  * <name>
//  *   DESCRIPTION: 
//  *   INPUTS: 
//  *   OUTPUTS: 
//  *   RETURN VALUE: 0 if success, -1 if anything bad happened
//  *   SIDE EFFECTS: none
//  */
// int32_t directory_read() {
//     return 0;
// }

// /* 
//  * <name>
//  *   DESCRIPTION: 
//  *   INPUTS: 
//  *   OUTPUTS: 
//  *   RETURN VALUE: 0 if success, -1 if anything bad happened
//  *   SIDE EFFECTS: none
//  */
// int32_t directory_write() {
//     return 0;
// }

// /* 
//  * <name>
//  *   DESCRIPTION: 
//  *   INPUTS: 
//  *   OUTPUTS: 
//  *   RETURN VALUE: 0 if success, -1 if anything bad happened
//  *   SIDE EFFECTS: none
//  */
// int32_t directory_close() {
//     return 0;
// }
