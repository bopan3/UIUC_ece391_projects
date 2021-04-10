/*
 * Functions of system call 
 */
#define FAIL -1
#define SUCCESS 0
#define FILENAME_LEN 32
#define TERM_LEN 128
#define VALIDATION_READ_SIZE 40
#include "sys_calls.h"
#include "types.h"
#include "file_sys.h"

/* 
 * 
 *   DESCRIPTION: 
 *   INPUTS: 
 *   OUTPUTS: 
 *   RETURN VALUE: 0 if success, -1 if anything bad happened
 *   SIDE EFFECTS: none
 */

int32_t execute(const uint8_t* command){
    uint8_t filename[FILENAME_LEN];     /* filename array */
    uint8_t args[TERM_LEN];             /* args array */
    int i;                              /* loop index */

    /* Sanity check */
    if (command == NULL) {
        return FAIL;
    }

    /* Parse command */
    if (FAIL == _parse_cmd_(command, filename, args)){
        return FAIL;
    }

    if (FAIL == _file_validation_(filename)){
        return FAIL;
    }
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

