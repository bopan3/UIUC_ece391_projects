/*
 * Functions of system call 
 */

#include "sys_calls.h"
#include "file_sys.h"
#include "terminal.h"
#include "rtc.h"

// For test
int32_t pid = 1;

/*
 *   open
 *   DESCRIPTION:
 *   INPUTS: fname - the file to open
 *   OUTPUTS:
 *   RETURN VALUE: 0 if success, -1 if anything bad happened
 *   SIDE EFFECTS:
 */
int32_t open(const uint8_t* fname){
    int i;                          // Loop index
    dentry_t *dentry = 0;           // pointer to dentry

    // if failed to find the entry, return -1
    if (-1 == read_dentry_by_name(fname, dentry))
        return -1;

    // get the pointer to current pcb
    pcb_block_t* cur_pcb = (pcb_block_t*)(0x800000 - 0x2000*(pid+1));

    // loop through all dynamic fds
    for (i = INI_FILES; i < N_FILES; ++i) {
        if(cur_pcb->file_array[i].flags == UNUSE) {
            /*
             * Initialize
                file_ops*   file_ops_ptr;
                uint32_t    idx_inode;
                uint32_t    file_pos;
                uint32_t    flages;
             */
            switch (dentry->f_type) {
                case FILE_RTC:
                    cur_pcb->file_array[i].file_ops_ptr = &rtc_fop_t;
                case FILE_DIREC:
                    cur_pcb->file_array[i].file_ops_ptr = &dir_fop_t;
                case FILE_REG:
                    cur_pcb->file_array[i].file_ops_ptr = &reg_fop_t;
            }
            cur_pcb->file_array[i].idx_inode = dentry->idx_inode;
            cur_pcb->file_array[i].file_pos = 0;        // 0 as the file has not been read yet
            cur_pcb->file_array[i].flags = INUSE;

            return 0;
        }
    }
    // exiting the while loop means all fds are full, fail
    return -1;
}

/*
 *   close
 *   DESCRIPTION:
 *   INPUTS: fd - the file descriptor to close
 *   OUTPUTS:
 *   RETURN VALUE: 0 if success, -1 if anything bad happened
 *   SIDE EFFECTS:
 */
int32_t close(int32_t fd){
    // fd number should be between 0 and 8
    if(fd < INI_FILES || fd > N_FILES)
        return -1;

    // find the current PCB
    pcb_block_t* cur_pcb = (pcb_block_t*)(0x800000 - 0x2000*(pid+1));

    // check if the current file descriptor is freed
    if(cur_pcb->file_array[fd].flags == UNUSE)
        return -1;

    // call the corresponding close function and check if success
    if (-1 == cur_pcb->file_array[fd].file_ops_ptr->close(fd))
        return -1;

    cur_pcb->file_array[fd].flags = UNUSE;

    return 0;
}

/*
 *   read
 *   DESCRIPTION:
 *   INPUTS: fd - the file descriptor to read from
 *           buf - the buffer to read from
 *           nbytes - number of bytes to read
 *   OUTPUTS:
 *   RETURN VALUE: 0 if success, -1 if anything bad happened
 *   SIDE EFFECTS: none
 */
int32_t read(int32_t fd, void* buf, int32_t nbytes){
    // fd number should be between 0 and 8
    if(fd < 0 || fd > N_FILES)
        return -1;
    // check if buf is NULL
    if (buf == NULL)
        return -1;
    // check if the number of bytes to read is legible
    if (nbytes < 0)
        return -1;

    // find the current PCB
    pcb_block_t* cur_pcb = (pcb_block_t*)(0x800000 - 0x2000*(pid+1));
    // check if the current file descriptor is in use
    if(cur_pcb->file_array[fd].flags == UNUSE)
        return -1;

    // read and return the number of bytes read
    int32_t ret = cur_pcb->file_array[fd].file_ops_ptr->read(fd,buf,nbytes);
    return ret;
}

/*
 *   write
 *   DESCRIPTION:
 *   INPUTS: fd - the file descriptor to write to
 *           buf - the buffer to write to
 *           nbytes - number of bytes to write
 *   OUTPUTS:
 *   RETURN VALUE: 0 if success, -1 if anything bad happened
 *   SIDE EFFECTS: none
 */
int32_t write(int32_t fd, const void* buf, int32_t nbytes) {
    // fd number should be between 0 and 8
    if(fd < 0 || fd > N_FILES)
        return -1;
    // check if buf is NULL
    if (buf == NULL)
        return -1;
    // check if the number of bytes to write is legible
    if (nbytes < 0)
        return -1;

    // find the current PCB
    pcb_block_t* cur_pcb = (pcb_block_t*)(0x800000 - 0x2000*(pid+1));
    // check if the current file descriptor is in use
    if(cur_pcb->file_array[fd].flags == UNUSE)
        return -1;

    // write and return the number of bytes wrote
    int32_t ret = cur_pcb->file_array[fd].file_ops_ptr->write(fd,buf,nbytes);
    return ret;
}

/*
 *   badread
 *   DESCRIPTION: bad
 *   INPUTS: none
 *   OUTPUTS:
 *   RETURN VALUE: -1 because I'm the bad thing itself
 *   SIDE EFFECTS: none
 */
int32_t badread(int32_t fd, void* buf, int32_t nbytes) {
    return -1;
}

/*
 *   badwrite
 *   DESCRIPTION: bad
 *   INPUTS: none
 *   OUTPUTS:
 *   RETURN VALUE: -1 because I'm the bad thing itself
 *   SIDE EFFECTS: none
 */
int32_t badwrite(int32_t fd, const void* buf, int32_t nbytes) {
    return -1;
}

void fop_t_init() {
    rtc_fop_t.read = rtc_read;
    rtc_fop_t.write = rtc_write;
    rtc_fop_t.open = rtc_open;
    rtc_fop_t.close = rtc_close;

    dir_fop_t.read = direct_read;
    dir_fop_t.write = direct_write;
    dir_fop_t.open = direct_open;
    dir_fop_t.close = direct_close;

    reg_fop_t.read = file_read;
    reg_fop_t.write = file_write;
    reg_fop_t.open = file_open;
    reg_fop_t.close = file_close;

    stdi_fop_t.read = terminal_read;
    stdi_fop_t.write = badwrite;
    stdi_fop_t.open = terminal_open;
    stdi_fop_t.close = terminal_close;

    stdo_fop_t.read = badread;
    stdo_fop_t.write = terminal_write;
    stdo_fop_t.open = terminal_open;
    stdo_fop_t.close = terminal_close;
}

