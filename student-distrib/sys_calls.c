/*
 * Functions of system call
 */

#include "sys_calls.h"
#include "file_sys.h"
#include "terminal.h"
#include "rtc.h"
#include "types.h"
#include "paging.h"
#include "scheduler.h"
/* Global Section */
int8_t task_array[MAX_PROC] = {0};  /* for hold PID */
int32_t pid = 0, new_pid = 0;       /* pid cursor */
extern terminal_t tm_array[];
extern int32_t terminal_tick;   
extern int32_t running_terminal;
/*
 *   getargs
 *   DESCRIPTION: copy program args from kernel to user
 *   INPUTS: buf - user space buf ptr
 *           nbytes - buf size
 *   OUTPUTS:
 *   RETURN VALUE: 0 if success, -1 if anything bad happened
 *   SIDE EFFECTS:
 */
int32_t getargs (uint8_t* buf, int32_t nbytes){

    // sti();

    pcb* cur_pcb_ptr;

    /* sanity check */
    if (buf == NULL) return SYS_CALL_FAIL;

    /* check if buf in user space */
    if ( ((int32_t) buf >= (int32_t) USER_PAGE_BASE) && (int32_t) buf + nbytes < USER_ESP){
        cur_pcb_ptr = get_pcb_ptr(pid);

        /* check if args exist */
        if (cur_pcb_ptr->args[0] == '\0') return SYS_CALL_FAIL;

        /* check if args longer than buf size */
        if (strlen((int8_t*)cur_pcb_ptr->args) > nbytes) return SYS_CALL_FAIL;

        /* Copy from kernel to user */
        strncpy((int8_t*)buf, (int8_t*)cur_pcb_ptr->args, nbytes);
        return SUCCESS;
    }

    return SYS_CALL_FAIL;
}

/*
 *   open
 *   DESCRIPTION:
 *   INPUTS: fname - the file to open
 *   OUTPUTS:
 *   RETURN VALUE: 0 if success, -1 if anything bad happened
 *   SIDE EFFECTS:
 */
int32_t open(const uint8_t* fname){

    // sti();

    int i;                          // Loop index
    dentry_t dentry;               // pointer to dentry

    // check if buf is NULL
    if (NULL == fname)
        return SYS_CALL_FAIL;

    // if failed to find the entry, return -1
    if (SYS_CALL_FAIL == read_dentry_by_name(fname, &dentry))
        return SYS_CALL_FAIL;

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
            switch (dentry.f_type) {
                case FILE_RTC:
                    cur_pcb->file_array[i].file_ops_ptr = &rtc_fop_t;
                    break;
                case FILE_DIREC:
                    cur_pcb->file_array[i].file_ops_ptr = &dir_fop_t;
                    break;
                case FILE_REG:
                    cur_pcb->file_array[i].file_ops_ptr = &reg_fop_t;
                    break;
            }
            cur_pcb->file_array[i].idx_inode = dentry.idx_inode;
            cur_pcb->file_array[i].file_pos = 0;        // 0 as the file has not been read yet
            cur_pcb->file_array[i].flags = INUSE;

            // call open for specific type
            cur_pcb->file_array[i].file_ops_ptr->open(fname);
            return i;
        }
    }
    // exiting the while loop means all fds are full, fail
    return SYS_CALL_FAIL;
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

    // sti();

    // fd number should be between 0 and 8
    if(fd < INI_FILES || fd > N_FILES)
        return SYS_CALL_FAIL;

    // find the current PCB
    pcb* cur_pcb = get_pcb_ptr(pid);

    // check if the current file descriptor is freed
    if(cur_pcb->file_array[fd].flags == UNUSE)
        return SYS_CALL_FAIL;

    // call the corresponding close function and check if success
    if (SYS_CALL_FAIL == cur_pcb->file_array[fd].file_ops_ptr->close(fd))
        return SYS_CALL_FAIL;

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

    sti();

    // fd number should be between 0 and 8
    if(fd < 0 || fd > N_FILES)
        return SYS_CALL_FAIL;
    // check if buf is NULL
    if (buf == NULL)
        return SYS_CALL_FAIL;
    // check if the number of bytes to read is legible
    if (nbytes < 0)         // if read negative number bytes, return fail
        return SYS_CALL_FAIL;

    // find the current PCB
    pcb* cur_pcb = get_pcb_ptr(pid);
    // check if the current file descriptor is in use
    if(cur_pcb->file_array[fd].flags == UNUSE)
        return SYS_CALL_FAIL;

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

    sti();

    // fd number should be between 0 and 8
    if(fd < 0 || fd > N_FILES)
        return SYS_CALL_FAIL;
    // check if buf is NULL
    if (buf == NULL)
        return SYS_CALL_FAIL;
    // check if the number of bytes to write is legible
    if (nbytes < 0)         // if write negative number bytes, return fail
        return SYS_CALL_FAIL;

    // find the current PCB
    pcb* cur_pcb = get_pcb_ptr(pid);
    // check if the current file descriptor is in use
    if(cur_pcb->file_array[fd].flags == UNUSE)
        return SYS_CALL_FAIL;

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
    return SYS_CALL_FAIL;
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
    return SYS_CALL_FAIL;
}

/*
 *   fop_t_init
 *   DESCRIPTION: initialize file operations table
 *   INPUTS: none
 *   OUTPUTS:
 *   RETURN VALUE: none
 *   SIDE EFFECTS: initialize file operations table
 */
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

/* Checkpoint 3.4 task */
/*
 *   vidmap
 *   DESCRIPTION: map the virtual space 0x8800000 to the physical vedio memory, and store 0x8800000 to  *screen_start in the userspace
 *   INPUTS: screen_start - the pointer point to the pointer point to the start of vedio memory in user space
 *   OUTPUTS:
 *   RETURN VALUE: 0 if success, -1 if anything bad happened
 *   SIDE EFFECTS: set up the page starting at virtual space 0x8800000
 */
int32_t vidmap (uint8_t** screen_start){
    // sti();
    // check whether the address falls within the address range covered by the single user-level page
    if (screen_start==NULL || (int) screen_start<USER_PAGE_BASE || (int) screen_start>=USER_PAGE_BASE+_4MB_-_4B_){   //also make sure the end of variable "screen_start" is not out of the user space
         return SYS_CALL_FAIL;
    }
    if ( pid == tm_array[terminal_tick].tm_pid) {paging_set_for_vedio_mem(VIRTUAL_ADDR_VEDIO_PAGE,VIDEO);}
    else if (terminal_tick==0){paging_set_for_vedio_mem(VIRTUAL_ADDR_VEDIO_PAGE,TERMINAL_1_ADDR);}
    else if (terminal_tick==1){paging_set_for_vedio_mem(VIRTUAL_ADDR_VEDIO_PAGE,TERMINAL_2_ADDR);}
    else if (terminal_tick==2){paging_set_for_vedio_mem(VIRTUAL_ADDR_VEDIO_PAGE,TERMINAL_3_ADDR);}
    else{ return SYS_CALL_FAIL;}
    *screen_start= (uint8_t*) VIRTUAL_ADDR_VEDIO_PAGE;
    return 0;
}

/*
 *   halt
 *   DESCRIPTION: halt a user program to return control back
 *   INPUTS: status - return value to execute
 *   OUTPUTS:
 *   RETURN VALUE: -1 if anything bad happened
 *   SIDE EFFECTS:
 */
int32_t halt(uint8_t status){

    cli();

    int i;              /* loop index */

    /* Get pcb info */
    pcb* cur_pcb_ptr = get_pcb_ptr(pid);
    pcb* prev_pcb_ptr;
    int32_t k_ebp, k_esp;

    /* intend to halt shell */
    if (cur_pcb_ptr->pid < running_terminal){
        /* then go back to shell */
        printf("[WARINING] FAIL TO HALT ROOT SHELL TASK\n");
        /*-------------------- Context switch micro --------------------*/
        pcb* cur_pcb = get_pcb_ptr(pid);    /* PCB for current PID */

        /* tss settings */
        tss.ss0 = KERNEL_DS;
        tss.esp0 = _8MB_ - (_8KB_ * pid) - 4;

        /* value for asm setting */
        uint32_t _0_SS = (uint32_t) USER_DS;
        uint32_t  ESP = (uint32_t) USER_ESP;
        uint32_t  _0_CS = (uint32_t) USER_CS;
        uint32_t  EIP = cur_pcb->user_eip;

        asm volatile ( "movl %%ebp, %0" : "=r"(cur_pcb->kernel_ebp_exc) );
        asm volatile ( "movl %%esp, %0" : "=r"(cur_pcb->kernel_esp_exc) );

        /* asm setting */
        asm volatile (
            "cli;"
            "movw    %%ax, %%ds;"
            "pushl   %%eax;"
            "pushl   %%ebx;"
            "pushfl  ;"
            "popl    %%edi;"
            "orl     $0x0200, %%edi;"
            "pushl   %%edi;"
            "pushl   %%ecx;"
            "pushl   %%edx;"
            "iret   ;"
        :   /* no outputs */
        : "a"(_0_SS), "b"(ESP), "c"(_0_CS), "d"(EIP)
        :   "edi"
        );
        /*--------------------------------------------------------------*/
    }

    /*  Restore parent data */
    prev_pcb_ptr = get_pcb_ptr(cur_pcb_ptr->prev_pid);
    task_array[pid] = 0;        /* release the pid entry at task array */
    pid = prev_pcb_ptr->pid;    /* update pid */
    tm_array[terminal_tick].tm_pid = pid; 
    
    /* tss update */
    tss.ss0 = KERNEL_DS;
    tss.esp0 = _8MB_ - (_8KB_ * (pid)) - 4;

    /* Restore parent paging */
    paging_set_user_mapping(pid);
    paging_restore_for_vedio_mem(VIRTUAL_ADDR_VEDIO_PAGE);

    /* Close any relevant FDs */
    /* close normal file */
    for (i = 2; i < N_FILES; i++){
        if (cur_pcb_ptr->file_array[i].flags == INUSE){
            close(i);
        }
    }
    /* close stdin, stdout */
    cur_pcb_ptr->file_array[0].flags = UNUSE;   /* stdi */
    cur_pcb_ptr->file_array[1].flags = UNUSE;   /* stdo */

    /* Jump to execute return */
    k_ebp = cur_pcb_ptr->kernel_ebp_exc;
    k_esp = cur_pcb_ptr->kernel_esp_exc;

    asm volatile(
        "xorl %%eax, %%eax;"
        "movb %0, %%al;"
        "movl %1, %%ebp;"
        "movl %2, %%esp;"
        "jmp return_from_halt;"
        : /* No output */
        : "r"(status), "r"(k_ebp), "r"(k_esp)
        : "esp", "ebp", "eax"
    );

    return SYS_CALL_FAIL;   /* if touch here, it must have something wrong */
}

/*
 *   execute
 *   DESCRIPTION: envoke a user program from kernel
 *   INPUTS: command - program name and args
 *   OUTPUTS:
 *   RETURN VALUE: 0 if success, -1 if anything bad happened
 *   SIDE EFFECTS:
 */
int32_t execute(const uint8_t* command){
    // sti();
    uint8_t filename[FILENAME_LEN];     /* filename array */
    uint8_t args[TERM_LEN];             /* args array */
    int32_t return_val;                 /* for return value from halt */
    int32_t eip;                        /* to get user program start address */
    int32_t k_ebp, k_esp;               

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
    switch ( _mem_setting_(filename, &eip)){
        case SYS_CALL_FAIL:
            return SYS_CALL_FAIL;
            break;

        case EXE_LIMIT:
            return SUCCESS;
            break;
    }

    /* setting PCB */
    if (SYS_CALL_FAIL == _PCB_setting_(filename, args, &eip)) return SYS_CALL_FAIL;

    /* context switch */
    /*-------------------- Context switch micro --------------------*/
    pcb* cur_pcb = get_pcb_ptr(pid);    /* PCB for current PID */

    /* tss settings */
    tss.ss0 = KERNEL_DS;
    tss.esp0 = _8MB_ - (_8KB_ * pid) - 4;

    /* value for asm setting */
    uint32_t _0_SS = (uint32_t) USER_DS;
    uint32_t  ESP = (uint32_t) USER_ESP;
    uint32_t  _0_CS = (uint32_t) USER_CS;
    uint32_t  EIP = cur_pcb->user_eip;

    
    asm volatile ( "movl %%ebp, %0" : "=r"(k_ebp) );
    asm volatile ( "movl %%esp, %0" : "=r"(k_esp) );
    cur_pcb->kernel_ebp_exc = k_ebp;
    cur_pcb->kernel_esp_exc = k_esp;
    

    /* asm setting */
    asm volatile (
        "cli;"
        "movw    %%ax, %%ds;"
        "pushl   %%eax;"
        "pushl   %%ebx;"
        "pushfl  ;"
        "popl    %%edi;"
        "orl     $0x0200, %%edi;"
        "pushl   %%edi;"
        "pushl   %%ecx;"
        "pushl   %%edx;"
        "iret   ;"
    :   /* no outputs */
    : "a"(_0_SS), "b"(ESP), "c"(_0_CS), "d"(EIP)
    :   "edi"
    );
    /*--------------------------------------------------------------*/

    // Position that halt() jumps to
    asm volatile (
        "return_from_halt:"
        "movl %%eax, %0;"
        : "=r"(return_val)
    );

    return return_val;
}



/* Signal Support for extra credit, just fake placeholder now */
int32_t set_handler (int32_t signum, void* handler_address){
    // sti();
    return SYS_CALL_FAIL;
}

int32_t sigreturn (void){
    // sti();
    return SYS_CALL_FAIL;
}

// =================== helper function ===============


/*
 *   exp_halt
 *   DESCRIPTION: halt from exception to return control back
 *   INPUTS: 
 *   OUTPUTS:
 *   RETURN VALUE: 
 *   SIDE EFFECTS:
 */
void exp_halt(){

    // sti();

    int i;              /* loop index */
    int32_t k_ebp, k_esp;

    /* Get pcb info */
    pcb* cur_pcb_ptr = get_pcb_ptr(pid);
    pcb* prev_pcb_ptr;

    // printf("===================================================A\n");

    /* intend to halt shell */
    if (cur_pcb_ptr->pid < running_terminal){
        /* then go back to shell */
        printf("[WARINING] FAIL TO HALT ROOT SHELL TASK\n");
        /*-------------------- Context switch micro --------------------*/
        pcb* cur_pcb = get_pcb_ptr(pid);    /* PCB for current PID */

        /* tss settings */
        tss.ss0 = KERNEL_DS;
        tss.esp0 = _8MB_ - (_8KB_ * pid) - 4;

        /* value for asm setting */
        uint32_t _0_SS = (uint32_t) USER_DS;
        uint32_t  ESP = (uint32_t) USER_ESP;
        uint32_t  _0_CS = (uint32_t) USER_CS;
        uint32_t  EIP = cur_pcb->user_eip;

        asm volatile ( "movl %%ebp, %0" : "=r"(cur_pcb->kernel_ebp_exc) );
        asm volatile ( "movl %%esp, %0" : "=r"(cur_pcb->kernel_esp_exc) );

        /* asm setting */
        asm volatile (
            "cli;"
            "movw    %%ax, %%ds;"
            "pushl   %%eax;"
            "pushl   %%ebx;"
            "pushfl  ;"
            "popl    %%edi;"
            "orl     $0x0200, %%edi;"
            "pushl   %%edi;"
            "pushl   %%ecx;"
            "pushl   %%edx;"
            "iret   ;"
        :   /* no outputs */
        : "a"(_0_SS), "b"(ESP), "c"(_0_CS), "d"(EIP)
        :   "edi"
        );
        /*--------------------------------------------------------------*/
    }

    /*  Restore parent data */
    prev_pcb_ptr = get_pcb_ptr(cur_pcb_ptr->prev_pid);
    task_array[pid] = 0;        /* release the pid entry at task array */
    pid = prev_pcb_ptr->pid;    /* update pid */
    tm_array[terminal_tick].tm_pid = pid; 

    /* tss update */
    tss.ss0 = KERNEL_DS;
    tss.esp0 = _8MB_ - (_8KB_ * (pid)) - 4;

    /* Restore parent paging */
    paging_set_user_mapping(pid);
    paging_restore_for_vedio_mem(VIRTUAL_ADDR_VEDIO_PAGE);

    /* Close any relevant FDs */
    /* close normal file */
    for (i = 2; i < N_FILES; i++){
        if (cur_pcb_ptr->file_array[i].flags == INUSE){
            close(i);
        }
    }
    /* close stdin, stdout */
    cur_pcb_ptr->file_array[0].flags = UNUSE;   /* stdi */
    cur_pcb_ptr->file_array[1].flags = UNUSE;   /* stdo */

    /* Jump to execute return */
    k_esp = cur_pcb_ptr->kernel_esp_exc;
    k_ebp = cur_pcb_ptr->kernel_ebp_exc;
    asm volatile(
        "xorl %%eax, %%eax;"
        "movl %0, %%eax;"
        "movl %1, %%ebp;"
        "movl %2, %%esp;"
        "jmp return_from_halt;"
        : /* No output */
        : "r"(256), "r"(k_ebp), "r"(k_esp)
        : "esp", "ebp", "eax"
    );

    return ;   /* if touch here, it must have something wrong */
}


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
    int cmd_len = strlen((int8_t*)(command));       /* length of command string */
    int filename_len = 0;                           /* length of file name of program */
    int arg_len = 0;                                /* length of args string */
    int i;                                          /* loop index through command string */

    /* Step 1: find program filename */
    /* strip the head space */
    for (i = 0; (i < cmd_len) && (command[i] == ' '); i++){}

    /* check if all command is just space */
    if (i == cmd_len) return SYS_CALL_FAIL;

    /* get the filename */
    while(command[i] != ' ' && command[i] != '\n' && command[i] != '\0'){
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

    /* Otherwise, copy the args and strip the end space */
    while(i < cmd_len && command[i] != ' ' ){
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
    dentry_t validation_dentry;                     /* dentry instance for validation */
    uint8_t validation_buf[VALIDATION_READ_SIZE];   /* buf for read from dentry */

    /* Check if file exist */
    if (SYS_CALL_FAIL == read_dentry_by_name(filename, &validation_dentry)) return SYS_CALL_FAIL;

    /* Valid if read dentry work */
    if (VALIDATION_READ_SIZE != read_data(validation_dentry.idx_inode, 0, validation_buf, VALIDATION_READ_SIZE)){
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
 *                  1 - for exe up limit
 *   SIDE EFFECTS:  none
 */
int32_t _mem_setting_(const uint8_t* filename, int32_t* eip){
    dentry_t den;               /* for loading user program */
    uint8_t* Loading_address;   /* as the buf to load program */
    int32_t i;                  /* loop index */

    /* 1. Find a free entry for new task */
    for (i = 0; i < MAX_PROC; i++){
        if (task_array[i] == 0){
            new_pid = i;
            task_array[i] = 1;
            tm_array[terminal_tick].tm_pid = new_pid;
            break;
        }
    }

    /* Check if new process request beyond ability */
    if (i == MAX_PROC) {
        printf("[WARINING] REACH MAXIMUM NESTED TASK #%d, FAIL TO EXECUTE NEW\n", MAX_PROC);
        return EXE_LIMIT;
    }

    /* 2. Mapping virtual 128 MB to physical image address */
    paging_set_user_mapping(new_pid);

    /* 3. Loading user program via read_data, copy from file system to memory */
    read_dentry_by_name(filename, &den);
    Loading_address = (uint8_t*)0x8048000; /* fixed address, according to Appendix C */
    read_data(den.idx_inode, 0, Loading_address, get_file_size(den.idx_inode));

    *eip = *(int32_t*)(Loading_address+24);
    return SUCCESS;

}

/*
 *  _PCB_setting_
 *   DESCRIPTION: helper function to set PCB struct
 *   INPUTS: filename - filename array
 *           args - to store in PCB
 *           eip - user eip value, to store in PCB
 *   OUTPUTS: none
 *   RETURN VALUE:  0 - for success
 *   SIDE EFFECTS:  none
 */
int32_t _PCB_setting_(const uint8_t* filename, const uint8_t* args, int32_t* eip){
    // int32_t kernel_ebp, kernel_esp; /* address value of two register */

    /* Getting pcb base address */
    pcb* new_pcb_ptr = get_pcb_ptr(new_pid);

    /* filed settings */
    new_pcb_ptr->pid = new_pid;
    new_pcb_ptr->prev_pid = pid;
    strncpy((int8_t*)(new_pcb_ptr->args), (int8_t*)(args), TERM_LEN); /* copy args to PCB */

    /* fd array initialization */
    _fd_init_(new_pcb_ptr);

    /* Regs info */
    new_pcb_ptr->user_eip = *eip;

    // rtc info setting
    new_pcb_ptr->rtc_opened=0;
    new_pcb_ptr->virtual_freq=0;  
    new_pcb_ptr->current_count=MAX_FREQ;
    new_pcb_ptr->virtual_iqr_got=0;        

    /* move the reg value to variable */
    // asm volatile ( "movl %%ebp, %0" : "=r"(kernel_ebp) );
    // asm volatile ( "movl %%esp, %0" : "=r"(kernel_esp) );
    // new_pcb_ptr->kernel_ebp_exc = kernel_ebp;
    // new_pcb_ptr->kernel_esp_exc = kernel_esp;
    // new_pcb_ptr->user_esp is always the same

    /* Finally, update global PID  */
    pid = new_pid;

    return SUCCESS;
}

/*
 *  _fd_init_
 *   DESCRIPTION: helper function to initialize new file array for new PCB
 *   INPUTS: pcb_addr - PCB struct address
 *   OUTPUTS: 
 *   RETURN VALUE: 
 *   SIDE EFFECTS:  none
 */
void _fd_init_(pcb* pcb_addr){
    int i;      /* loop index */

    /* initialize the array in new PCB */
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

            default:    /* the rest are not used yet */
                pcb_addr->file_array[i].flags = UNUSE;
        }
    }

}




/*
 *  get_pcb_ptr
 *   DESCRIPTION: shortcut function to calculate PCB start address for given PID
 *   INPUTS: pid - task identifier
 *   OUTPUTS: 
 *   RETURN VALUE: PCB start address for given PID
 *   SIDE EFFECTS:  
 */
pcb* get_pcb_ptr(int32_t pid){
    return (pcb*)(_8MB_ - _8KB_ *(pid + 1));
}
