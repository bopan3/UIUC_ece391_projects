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

/* Initialize a file discriptor array here (only for check piont 2) */
static struct file_des_t file_array[8];       // temporarily used descriptor array
static int32_t file_count;                    // count number of file opened
static int32_t direct_read_count;             // count the file name to be read

/* Some parameters */
#define STR_LEN 32
#define BLOCK_SIZE 4096

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

    direct_read_count = 0;
    
    file_count = 2;     // only used for check point 2
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
            dentry->f_size = p_inode[p_dentry[i].idx_inode].length;
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
    dentry->f_size = p_inode[p_dentry[index].idx_inode].length;

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

    struct dentry_t result;     // structure to store the search result

    // Check current number of opened files
    if (file_count >= N_FILES) {
        return -1;
    }

    // Search the file
    if (-1 == read_dentry_by_name(filename, &result)) {
        return -1;
    }

    // Add a new descriptor to the array
    file_array[file_count].idx_inode = result.idx_inode;
    file_array[file_count].file_type = result.f_type;
    file_array[file_count].file_pos = 0;
    file_array[file_count].flages = 0;

    file_count += 1;
    return (file_count - 1);
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
int32_t file_read(int32_t fd, uint8_t* buf, int32_t nbytes) {

    uint32_t idx_inode;     // index of inode block
    uint32_t offset;        // starting address in file
    uint32_t f_size;        // file size
    int32_t length;         // length of reading

    // Check discriptor
    if (fd < 0 || fd >= N_FILES) {
        return -1;
    }

    // Check file type
    if (file_array[fd].file_type != 2) {
        return -1;
    }

    // Calculate parameters
    idx_inode = file_array[fd].idx_inode;
    offset = file_array[fd].file_pos;
    f_size = p_inode[idx_inode].length;
    if (offset + nbytes > f_size) {
        nbytes = f_size - offset;
    }

    // Read the file
    length = read_data(idx_inode, offset, buf, nbytes);
    if (length >= 0) {
        file_array[fd].file_pos += length;
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
int32_t file_write(int32_t fd, uint8_t* buf, int32_t nbytes) {
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

    // Preserve first two slot for stdin, stdout
    if (fd < 2 || file_count <= 2 || fd > 7) {
        return -1;
    }

    file_array[fd].idx_inode = 0;
    file_array[fd].file_pos = 0;
    file_array[fd].flages = 0;
    file_array[fd].file_type = 0;

    file_count -= 1;

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
    
    struct dentry_t result;     // structure to store the search result

    // Check current number of opened files
    if (file_count >= N_FILES) {
        return -1;
    }

    // Search the file
    if (-1 == read_dentry_by_name(directname, &result)) {
        return -1;
    }

    // Check type
    if (result.f_type != 1) {
        return -1;
    }

    return 0;
}

/* 
 * direct_read
 *   DESCRIPTION: read the file name
 *   INPUTS: fd - file descriptor
 *           buf - buffer that store the data to read
 *           nbytes - number of bytes to read
 *   OUTPUTS: buf which stores the file name
 *   RETURN VALUE: 1 if success, 0 if reach the end
 *   SIDE EFFECTS: none
 */
int32_t direct_read(uint8_t* buf) {
    
    uint32_t idx_inode;     // index of inode block
    uint32_t i;             // loop counter
    struct dentry_t temp;   // store temp dentry in loop

    if (0 != read_dentry_by_index(direct_read_count, &temp)) {
        return 0;
    }
    
    // Read file name
    strncpy((int8_t*)buf, (int8_t*)temp.f_name, 32);
    buf[33] = '\0';

    direct_read_count += 1;
    return 1;
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
int32_t direct_write(int32_t fd, uint8_t* buf, int32_t nbytes) {
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
