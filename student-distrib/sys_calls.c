/*
 * Functions of system call 
 */



#include "sys_calls.h"
#include "types.h"
#include "file_sys.h"
#include "paging.h"

int8_t task_array[MAX_PROC] = {0}; 
int32_t pid, new_pid;   

/* 
 * 
 *   DESCRIPTION: 
 *   INPUTS: 
 *   OUTPUTS: 
 *   RETURN VALUE: 0 if success, -1 if anything bad happened
 *   SIDE EFFECTS: none
 */
int32_t halt(uint8 t status){
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
        "movb %0, %%eax;"
        "movl %1, %%ebp;"
        "movl %2, %%esp;"
        "leave;"
        "ret;"
        : /* No output */
        : "r"(status), "r"(cur_pcb_ptr->kernel_ebp), "r"(cur_pcb_ptr->kernel_esp)
        : "esp", "ebp", "eax"   
    )
}

int32_t execute(const uint8_t* command){
    uint8_t filename[FILENAME_LEN];     /* filename array */
    uint8_t args[TERM_LEN];             /* args array */
    uint8_t eip_buf[USER_START_SIZE];   /* buffer to store the start address of program */
    int i;                              /* loop index */
    pcb* cur_pcb;                       /* for getting pcb ptr of current pid */
    /* Sanity check */
    if (command == NULL) {
        return FAIL;
    }

    /* Parse command */
    if (FAIL == _parse_cmd_(command, filename, args)){
        return FAIL;
    }

    /* Verify the filename */
    if (FAIL == _file_validation_(filename)){
        return FAIL;
    }

    /* settting memory part */
    if (FAIL == _mem_setting_(filename, args)) return FAIL;


    /* setting PCB */
    if (FAIL == _PCB_setting_(filename, args, eip_buf));

    /* context switch */
    pcb* cur_pcb = get_pcb_ptr(pid);
    // pcb* prev_pcb = get_pcb_ptr(cur_pcb->prev_pid);

    _ASM_switch_((uint32_t)USER_DS, (uint32_t) USER_ESP, (uint32_t) USER_CS, cur_pcb->user_eip);
    
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
int32_t _parse_cmd_(uint8_t* command, uint8_t* filename, uint8_t* args){
    int cmd_len = strlen(command);          /* length of command string */
    int filename_len = 0;                   /* length of file name of program */
    int arg_len = 0;                        /* length of args string */        
    int i;                                  /* loop index through command string */

    /* Step 1: find program filename */
    /* strip the head space */
    for (int i = 0; (i < cmd_len) && (command[i] == ' '); i++){}

    /* check if all command is just space */
    if (i == cmd_len) return FAIL;

    /* get the filename */
    while(command[i] != ' '){
        /* check if length of file name exceed */
        if (filename_len == FILENAME_LEN) return FAIL;

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
    if (FAIL == read_dentry_by_name(filename, validation_dentry)) return FAIL;

    /* Valid if read dentry work */
    if (VALIDATION_READ_SIZE != read_data(validation_dentry->idx_inode, 0, 
                                            validation_buf, VALIDATION_READ_SIZE)){
        return FAIL;
    }

    /* Check Magic Number at Head to verify if the file is executable */
    /* Magic Number of 4 Byte from Appendix C, 
        only if the head 4 Byte is same then it is executable */
    if (validation_buf[0] != 0x7f || validation_buf[1] != 0x45 || 
        validation_buf[2] != 0x4c || validation_buf[3] != 0x46){
            return FAIL;
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
    pcb* new_pcb_ptr;           

    /* 1. Find a free entry for new task */
    for (i = 0; i < MAX_PROC; i++){
        if (task_array[i] == 0){
            new_pid = i;
            task_array[i] = 1; 
            break;
        }
    }

    /* Check if new process request beyond ability */
    if (i == MAX_PROC) return FAIL;

    /* 2. Mapping virtual 128 MB to physical image address */
    paging_set_user_mapping(new_pid);

    /* 3. Loading user program via read_data, copy from file system to memory */
    read_dentry_by_name(filename, den);
    Loading_address = (uint8_t*)0x804800; /* fixed address */
    read_data(den->idx_inode, 0, Loading_address, den->f_size); 
    strncpy(eip_buf, Loading_address+24, USER_START_SIZE); /* Byte 24 - 27 is the address for program start */

    return SUCCESS;

}

int32_t _PCB_setting_(const uint8_t* filename, const uint8_t* args, uint8_t* eip_buf){
    
    /* Getting pcb base address */
    pcb* new_pcb_ptr = get_pcb_ptr(new_pid);
 
    /* filed settings */
    new_pcb_ptr->pid = new_pid;
    new_pcb_ptr->prev_pid = pid;
    strncpy(new_pcb_ptr->args, args, TERM_LEN); /* copy args to PCB */

    /* fd array initialization */
    _fd_init_(new_pcb_ptr);

    /* Regs info */
    new_pcb_ptr->user_eip = *((uint32_t*) eip_buf);
    // new_pcb_ptr->user_esp = 


    /* Finally, update global PID  */
    pid = new_pid;

}
void _fd_init_(pcb* pcb_addr){
    int i;      /* loop index */
    for (i = 0; i < N_FILES; i++){
        switch (i){
            case 0: /* stdin */
                pcb_addr->file_array[i].fop_t = &stdi_fop_t; 
                pcb_addr->file_array[i].flags = INUSE;
                pcb_addr->file_array[i].idx_inode = INVALID_NODE;
                pcb_addr->file_array[i].file_pos = 0;    /* Not matter */
                break;
            case 1: /* stdout */
                pcb_addr->file_array[i].fop_t = &stdo_fop_t;
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

    _ASM_switch_((uint32_t)USER_DS, (uint32_t) USER_ESP, (uint32_t) USER_CS, cur_pcb->user_eip);

    return SUCCESS; 
}


pcb* get_pcb_ptr(int32_t pid){
    return (pcb*)_8MB_ - _8KB_ *(pid + 1);
}

