/*
 * Functions of file system
 */

#include "file_sys.h"
#include "sys_calls.h"
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
#define BLOCK_SIZE 4096

extern int32_t pid;     // current number of process, from system call

/*-------------------- Helper functions --------------------*/ 

/* Note
 * 1. file_sys_addr indicate the starting address of file system 
 * 2. Cases that should return -1
 *      a. file name does not exists (read_dentry_by_name)
 *      b. invalde file index (read_dentry_by_index)
 *      c. inode number out of range (read_data)
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
    p_data = ((data_block_t*)file_sys_addr) + n_inode_b + 1;      // skip the boot block and inode blocks
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

    int32_t i, j;               // Loop counter
    uint8_t* fname_d;           // store name in dentry temporally
    int32_t find_flage = 0;
    int32_t length = strlen((int8_t*)fname);    // without '\0'

    // Check the length of input string
    if (length > STR_LEN) {
        return -1;
    }

    // Go through all dentries
    for (i = 0; i < n_dentry_b; i++) {

        fname_d = (uint8_t*)(p_dentry[i].f_name);
        find_flage = 1;

        // compare the name until a different characters or number 32 is reached
        for (j = 0; j < length; j++) {
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
 * read_dentry_by_index
 *   DESCRIPTION: find the dir entry at given index
 *   INPUTS: index - index in boot block
 *           dentry - structure to pass output
 *   OUTPUTS: dentry structure
 *   RETURN VALUE: 0 if success, -1 if anything bad happened
 *   SIDE EFFECTS: none
 */
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry) {

    int32_t length = strlen((int8_t*)(p_dentry[index].f_name));    // without '\0'

    // Check index
    if (index < 0 || index >= n_dentry_b) {
        return -1;
    }

    strncpy((int8_t*)(dentry->f_name), (int8_t*)(p_dentry[index].f_name), length);
    if (length < STR_LEN)
        dentry->f_name[length] = '\0';
    dentry->f_type = p_dentry[index].f_type;
    dentry->idx_inode = p_dentry[index].idx_inode; 

    return 0;
}

/* 
 * read_data
 *   DESCRIPTION: read data in certain file
 *   INPUTS: inode - inode number of file
 *           offset - starting index (in byte) of reading
 *           buf - buffer that stores the data
 *           length - amount of bytes to read
 *   OUTPUTS: buf that contains data
 *   RETURN VALUE: number of bytes readed, -1 if anything bad happened
 *   SIDE EFFECTS: none
 */
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length) {

    uint32_t byte_count = 0;            // number of bytes being readed
    uint32_t start_block, end_block;    // index of start and end data blocks (0 based, both edge included)
    uint32_t start_idx, end_idx;        // start and end index within the block (0 based, only lower edge included)
    uint32_t i, j;                      // loop counter
    inode_block_t file;
    uint32_t file_size;                 // size of file in terms of blocks
    uint32_t idx_data_block;
    data_block_t* data_block;           // pointer to the data block 

    if (buf == NULL) {
        return -1;
    }

    // Check inode number
    if (inode < 0 || inode >= n_inode_b) {
        return -1;
    }
    file = p_inode[inode];
    file_size = file.length / BLOCK_SIZE;
    if (file.length % BLOCK_SIZE != 0) {
        file_size += 1;
    }

    // Check range of reading (only lower boundary)
    if ((length + offset) < 0 || offset < 0 || length < 0) {
        return -1;
    }

    // Calculate start and end index
    start_block = offset / BLOCK_SIZE;
    end_block = (length + offset) / BLOCK_SIZE;
    start_idx = offset % BLOCK_SIZE;
    end_idx = (length + offset) % BLOCK_SIZE;

    // Loop through all relevant blocks
    for (i = start_block; i <= end_block; i++) {

        // Check file (upper) boudary
        if (i >= file_size) {
            return byte_count;
        }

        // Check whether the corresponding data block exists
        idx_data_block = file.idx_block[i];
        if (idx_data_block < 0 || idx_data_block >= n_data_block_b) {
            return -1;
        }
        data_block = p_data + idx_data_block;

        // Read data to buffer
        if (i == start_block && i == end_block) {
            // Read part of single block
            for (j = start_idx; j < end_idx; j++) {
                buf[byte_count] = data_block->data[j];
                byte_count += 1;
            }
        } else if (i == start_block) {
            // Read to the end of block
            for (j = start_idx; j < BLOCK_SIZE; j++) {
                buf[byte_count] = data_block->data[j];
                byte_count += 1;
            } 
        } else if (i == end_block) {
            // Read from start of the block
            for (j = 0; j < end_idx; j++) {
                buf[byte_count] = data_block->data[j];
                byte_count += 1;
            } 
        } else {
            // Read whole block
            // Read from start of the block
            for (j = 0; j < BLOCK_SIZE; j++) {
                buf[byte_count] = data_block->data[j];
                byte_count += 1;
            } 
        }
    }

    return byte_count;
}

/*-------------------- Wrapper functions --------------------*/ 

/* 
 * file_open
 *   DESCRIPTION: open the file, initialize any temporary structures
 *   INPUTS: filename - file name
 *   OUTPUTS: none
 *   RETURN VALUE: 0 if success, -1 if anything bad happened
 *   SIDE EFFECTS: none
 */
int32_t file_open(const uint8_t* filename) {
    return 0;
}

/* 
 * file_read
 *   DESCRIPTION: read content of file
 *   INPUTS: fd - file discriptor
 *           buf - store the content of reading
 *           nbytes - number of bytes to read
 *   OUTPUTS: nbytes of file contents
 *   RETURN VALUE: 0 if success, -1 if anything bad happened
 *   SIDE EFFECTS: none 
 */
int32_t file_read(int32_t fd, void* buf, int32_t nbytes) {

    uint32_t idx_inode;     // index of inode block
    uint32_t offset;        // starting address in file
    uint32_t f_size;        // file size
    int32_t length;         // length of reading

    if (buf == NULL) {
        return -1;
    }

    // Find the pcb
    pcb* cur_pcb = (pcb*)(_8MB_ - _8KB_*(pid+1));

    // Calculate parameters
    idx_inode = cur_pcb->file_array[fd].idx_inode;
    offset = cur_pcb->file_array[fd].file_pos;
    f_size = p_inode[idx_inode].length;
    if (offset + nbytes > f_size) {
        nbytes = f_size - offset;
    }

    // Read the file
    length = read_data(idx_inode, offset, (uint8_t*)buf, nbytes);
    if (length >= 0) {
        cur_pcb->file_array[fd].file_pos += length;
    }
    
    return length;
}

/* 
 * file_write
 *   DESCRIPTION: write content to file (but should do nothing in this mp)
 *   INPUTS: fd - file descriptor
 *           buf - content to write to file
 *           nbytes - length of content
 *   OUTPUTS: none
 *   RETURN VALUE: -1
 *   SIDE EFFECTS: none
 */
int32_t file_write(int32_t fd, const void* buf, int32_t nbytes) {
    return -1;
}

/* 
 * file_close
 *   DESCRIPTION: undo the process in file_open
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: 0 if success, -1 if anything bad happened
 *   SIDE EFFECTS: none
 */
int32_t file_close(int32_t fd) {
    return 0;
}

/* 
 * direct_open
 *   DESCRIPTION: open a directory
 *   INPUTS: directname - name of directory
 *   OUTPUTS: none
 *   RETURN VALUE: 0 if success, -1 if anything bad happened
 *   SIDE EFFECTS: none
 */
int32_t direct_open(const uint8_t* directname) {
    return 0;
}

/* 
 * direct_read
 *   DESCRIPTION: read the file name (it is user's responsible to ensure that buffer is larger than 33)
 *   INPUTS: fd - file descriptor
 *           buf - buffer that store the data to read
 *           nbytes - number of bytes to read
 *   OUTPUTS: buf which stores the file name
 *   RETURN VALUE: 1 if success, 0 if reach the end, -1 of fail
 *   SIDE EFFECTS: none
 */
int32_t direct_read(int32_t fd, void* buf, int32_t nbytes) {

    struct dentry_t temp;   // store temp dentry in loop
    int8_t* string = buf;

    if (buf == NULL) {
        return -1;
    }

    // Find the pcb
    pcb* cur_pcb = (pcb*)(_8MB_ - _8KB_*(pid+1));

    if (0 != read_dentry_by_index(cur_pcb->file_array[fd].file_pos, &temp)) {
        return 0;
    }
    
    cur_pcb->file_array[fd].file_pos += 1;

    // Read file name
    strncpy((int8_t*)buf, (int8_t*)temp.f_name, 33);
    string[32] = '\0';

    return strlen(buf);
}

/* 
 * direct_write
 *   DESCRIPTION: do nothing
 *   INPUTS: fd - file descriptor
 *           buf - buffer that store the data to write
 *           nbytes - number of bytes to write
 *   OUTPUTS: none
 *   RETURN VALUE: -1
 *   SIDE EFFECTS: none
 */
int32_t direct_write(int32_t fd, const void* buf, int32_t nbytes) {
    return -1;
}

/* 
 * direct_close
 *   DESCRIPTION: do nothing
 *   INPUTS: fd - file descriptor
 *   OUTPUTS: none
 *   RETURN VALUE: 0
 *   SIDE EFFECTS: none
 */
int32_t direct_close(int32_t fd) {
    return 0;
}


/*-------------------- Assistance functions --------------------*/ 

/* 
 * get_file_size
 *   DESCRIPTION: get size of file
 *   INPUTS: inode - number of inode
 *   OUTPUTS: none
 *   RETURN VALUE: size of file if success, -1 if anything bad happened
 *   SIDE EFFECTS: none
 */
int32_t get_file_size(uint32_t inode) {
    if (inode >= n_inode_b)
        return -1;
    return p_inode[inode].length;
}

