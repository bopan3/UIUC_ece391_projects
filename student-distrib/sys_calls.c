/*
 * Functions of system call
 */



#include "sys_calls.h"
#include "file_sys.h"
#include "terminal.h"
#include "rtc.h"
#include "types.h"
#include "paging.h"

int8_t task_array[MAX_PROC] = {0};
int32_t pid, new_pid;

// For test only
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
    pcb* cur_pcb = get_pcb_ptr(pid);

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

            return i;
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
    pcb* cur_pcb = get_pcb_ptr(pid);

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
    pcb* cur_pcb = get_pcb_ptr(pid);
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
    pcb* cur_pcb = get_pcb_ptr(pid);
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

int32_t halt(uint8_t status){
    int i;              /* loop index */

    /* Get pcb info */
    pcb* cur_pcb_ptr = get_pcb_ptr(pid);
    pcb* prev_pcb_ptr;

    /* intend to halt shell */
    if (cur_pcb_ptr->pid == ROOT_TASK){
        /* then go back to shell */
        /* TODO */
    }

    /*  Restore parent data */
    prev_pcb_ptr = get_pcb_ptr(cur_pcb_ptr->prev_pid);
    task_array[pid] = 0;        /* release the pid entry at task array */
    pid = prev_pcb_ptr->pid;    /* update pid */

    /* tss update */
    tss.ss0 = KERNEL_DS;
    tss.esp0 = _8MB_ - (_8KB_ * (pid+1)) - 4;   /*  */

    /* Restore parent paging */
    paging_set_user_mapping(pid);

    /* Close any relevant FDs */
    /* close normal file */
    for (i = 2; i < N_FILES; i++){
        if (cur_pcb_ptr->file_array[i].flags == INUSE){
            close(i);
        }
    }
    /* close stdin, stdout */
    /* TODO */

    /* Jump to execute return */

    asm volatile(
        "xorl %%eax, %%eax;"
        "movb %0, %%al;"
        "movl %1, %%ebp;"
        "movl %2, %%esp;"
        "leave;"
        "ret;"
        : /* No output */
        : "r"(status), "r"(cur_pcb_ptr->kernel_ebp), "r"(cur_pcb_ptr->kernel_esp)
        : "esp", "ebp", "eax"
    );
    
    return SYS_CALL_FAIL;   /* if touch here, it must have something wrong */
}

int32_t execute(const uint8_t* command){
    uint8_t filename[FILENAME_LEN];     /* filename array */
    uint8_t args[TERM_LEN];             /* args array */
    uint8_t eip_buf[USER_START_SIZE];   /* buffer to store the start address of program */
    pcb* cur_pcb;                       /* for getting pcb ptr of current pid */

    /* Sanity check */
    if (command == NULL) {
        return SYS_CALL_FAIL;
    }

    /* Parse command */
    if (SYS_CALL_FAIL == _parse_cmd_(command, filename, args)){
        return SYS_CALL_FAIL;
    }

    /* Verify the filename */
    if (SYS_CALL_FAIL == _file_validation_(filename)){
        return SYS_CALL_FAIL;
    }

    /* settting memory part */
    if (SYS_CALL_FAIL == _mem_setting_(filename, args)) return SYS_CALL_FAIL;


    /* setting PCB */
    if (SYS_CALL_FAIL == _PCB_setting_(filename, args, eip_buf));

    /* context switch */
    cur_pcb = get_pcb_ptr(pid);
    // pcb* prev_pcb = get_pcb_ptr(cur_pcb->prev_pid);

//    _ASM_switch_(_0_SS, ESP, _0_CS, EIP);
    _context_switch_();
    return SUCCESS; /* IRET in switch */
}


// =================== helper function ===============
/*
 * _parse_cmd_
 *   DESCRIPTION: helper function to parse the command sting to filename and args
 *   INPUTS: command - command array
 *           filename - to store parsed filename array
 *           args - to store parsed args array
 *   OUTPUTS: none
 *   RETURN VALUE: -1 - for invalid command parsed result
 *                  0 - for success
 *   SIDE EFFECTS:  none
 */
int32_t _parse_cmd_(const uint8_t* command, uint8_t* filename, uint8_t* args){
    int cmd_len = strlen((int8_t*)(command));          /* length of command string */
    int filename_len = 0;                   /* length of file name of program */
    int arg_len = 0;                        /* length of args string */
    int i;                                  /* loop index through command string */

    /* Step 1: find program filename */
    /* strip the head space */
    for (i = 0; (i < cmd_len) && (command[i] == ' '); i++){}

    /* check if all command is just space */
    if (i == cmd_len) return SYS_CALL_FAIL;

    /* get the filename */
    while(command[i] != ' '){
        /* check if length of file name exceed */
        if (filename_len == FILENAME_LEN) return SYS_CALL_FAIL;

        /* copy filename */
        filename[filename_len] = command[i];
        i++;
        filename_len++;
    }
    /* End the filename */
    filename[filename_len] = '\0';


    /* Step 2: find the program args */
    /* skip the space between filename and first args */
    for (; (i < cmd_len) && (command[i] == ' '); i++ ){}

    /* check if all args just space */
    if (i == cmd_len){
        args[0] = '\0';
        return SUCCESS;
    }

    /* Otherwise, copy the args */
    while(i < cmd_len){
        args[arg_len] = command[i];
        i++;
        arg_len++;
    }
    /* End the args */
    args[arg_len] = '\0';
    return SUCCESS;
}

/*
 * _file_validation_
 *   DESCRIPTION: helper function to verify the file
 *   INPUTS: filename - filename array
 *   OUTPUTS: none
 *   RETURN VALUE: -1 - for invalid result
 *                  0 - for success
 *   SIDE EFFECTS:  none
 */
int32_t _file_validation_(const uint8_t* filename){
    dentry_t* validation_dentry;
    uint8_t validation_buf[VALIDATION_READ_SIZE];

    /* Check if file exist */
    if (SYS_CALL_FAIL == read_dentry_by_name(filename, validation_dentry)) return SYS_CALL_FAIL;

    /* Valid if read dentry work */
    if (VALIDATION_READ_SIZE != read_data(validation_dentry->idx_inode, 0,
                                            validation_buf, VALIDATION_READ_SIZE)){
        return SYS_CALL_FAIL;
    }

    /* Check Magic Number at Head to verify if the file is executable */
    /* Magic Number of 4 Byte from Appendix C,
        only if the head 4 Byte is same then it is executable */
    if (validation_buf[0] != 0x7f || validation_buf[1] != 0x45 ||
        validation_buf[2] != 0x4c || validation_buf[3] != 0x46){
            return SYS_CALL_FAIL;
        }

    return SUCCESS;
}

/*
 * _mem_setting_
 *   DESCRIPTION: helper function to verify the file
 *   INPUTS: filename - filename array
 *   OUTPUTS: none
 *   RETURN VALUE: -1 - for invalid result
 *                  0 - for success
 *   SIDE EFFECTS:  none
 */
int32_t _mem_setting_(const uint8_t* filename, uint8_t* eip_buf){
    dentry_t* den;              /* for loading user program */
    uint8_t* Loading_address;   /* as the buf to load program */
    // int32_t pid;                /* PID for the new process */
    int32_t i;                  /* loop index */

    /* 1. Find a free entry for new task */
    for (i = 0; i < MAX_PROC; i++){
        if (task_array[i] == 0){
            new_pid = i;
            task_array[i] = 1;
            break;
        }
    }

    /* Check if new process request beyond ability */
    if (i == MAX_PROC) return SYS_CALL_FAIL;

    /* 2. Mapping virtual 128 MB to physical image address */
    paging_set_user_mapping(new_pid);

    /* 3. Loading user program via read_data, copy from file system to memory */
    read_dentry_by_name(filename, den);
    Loading_address = (uint8_t*)0x804800; /* fixed address */
    read_data(den->idx_inode, 0, Loading_address, get_file_size(den->idx_inode));
    strncpy((int8_t*)(eip_buf), (int8_t*)(Loading_address+24), USER_START_SIZE); /* Byte 24 - 27 is the address for program start */

    return SUCCESS;

}

int32_t _PCB_setting_(const uint8_t* filename, const uint8_t* args, uint8_t* eip_buf){

    /* Getting pcb base address */
    pcb* new_pcb_ptr = get_pcb_ptr(new_pid);

    /* filed settings */
    new_pcb_ptr->pid = new_pid;
    new_pcb_ptr->prev_pid = pid;
    strncpy((int8_t*)(new_pcb_ptr->args), (int8_t*)(args), TERM_LEN); /* copy args to PCB */

    /* fd array initialization */
    _fd_init_(new_pcb_ptr);

    /* Regs info */
    new_pcb_ptr->user_eip = *((uint32_t*) eip_buf);
    // new_pcb_ptr->user_esp =


    /* Finally, update global PID  */
    pid = new_pid;

    return SUCCESS;
}

void _fd_init_(pcb* pcb_addr){
    int i;      /* loop index */
    for (i = 0; i < N_FILES; i++){
        switch (i){
            case 0: /* stdin */
                pcb_addr->file_array[i].file_ops_ptr = &stdi_fop_t;
                pcb_addr->file_array[i].flags = INUSE;
                pcb_addr->file_array[i].idx_inode = INVALID_NODE;
                pcb_addr->file_array[i].file_pos = 0;    /* Not matter */
                break;
            case 1: /* stdout */
                pcb_addr->file_array[i].file_ops_ptr = &stdo_fop_t;
                pcb_addr->file_array[i].flags = INUSE;
                pcb_addr->file_array[i].idx_inode = INVALID_NODE;
                pcb_addr->file_array[i].file_pos = 0;    /* Not matter */
                break;

        default:
                pcb_addr->file_array[i].flags = UNUSE;
        }
    }
}

void _context_switch_(){
    pcb* cur_pcb = get_pcb_ptr(pid);
    // pcb* prev_pcb = get_pcb_ptr(cur_pcb->prev_pid);

    uint32_t _0_SS = (uint32_t) USER_DS;
    uint32_t  ESP = (uint32_t) USER_ESP;
    uint32_t  _0_CS = (uint32_t) USER_CS;
    uint32_t  EIP = cur_pcb->user_eip;

//    _ASM_switch_((uint32_t)USER_DS, (uint32_t) USER_ESP, (uint32_t) USER_CS, cur_pcb->user_eip);
//    _switch_(_0_SS, ESP, _0_CS, EIP);
    asm volatile (
        "pushl   %%eax;"
        "pushl   %%ebx;"
        "pushfl  ;"
        "pushl   %%ecx;"
        "pushl   %%edx;"
        "iret   ;"
    :   /* no outputs */
    : "a"(_0_SS), "b"(ESP), "c"(_0_CS), "d"(EIP)
    :   "memory"
    );
//    return SUCCESS;
}


pcb* get_pcb_ptr(int32_t pid){
    return (pcb*)_8MB_ - _8KB_ *(pid + 1);
}
