#ifndef _SYS_CALLS_H
#define _SYS_CALLS_H

#include "types.h"

/* Some parameters */
#define N_FILES     8
#define INI_FILES   2

#define INUSE   1
#define UNUSE   0

#define _8MB_ 0x800000
#define _8KB_ 0x2000
#define SYS_CALL_FAIL -1
#define SUCCESS 0
#define FILENAME_LEN 32
#define TERM_LEN 128
#define VALIDATION_READ_SIZE 40
#define MAX_PROC 6
#define INVALID_NODE -1
#define USER_START_SIZE 4
#define ROOT_TASK -1
#define USER_PAGE_BASE 0x8000000
#define USER_ESP (USER_PAGE_BASE + 0x400000 - 4) /* 128 MB for start user + 4 MB for page size - 4 entry */
#define EXE_LIMIT 1
#define VIRTUAL_ADDR_VEDIO_PAGE 0x8800000
#define MAX_FREQ 1024 

/* File operation tables */
// typedef struct file_ops_t {
//     /* TODO */
// } file_ops_t;

#define FILE_RTC    0
#define FILE_DIREC  1
#define FILE_REG    2

/* File Operations Table Pointer */
typedef struct fop_t {
    int32_t (*read)(int32_t fd, void* buf, int32_t nbytes);
    int32_t (*write)(int32_t fd, const void* buf, int32_t nbytes);
    int32_t (*open)(const uint8_t* fname);
    int32_t (*close)(int32_t fd);
} fop_t;

/* File descriptor */
typedef struct file_des_t {
    fop_t*    file_ops_ptr;
    uint32_t    idx_inode;
    uint32_t    file_pos;   // position where last read ends
    uint32_t    flags;     // flages that indicate file's state
} file_des_t;

/* PCB struct */
typedef struct pcb {
    file_des_t  file_array[N_FILES];
    uint8_t args[TERM_LEN];
    uint32_t pid;
    uint32_t prev_pid;

    // uint32_t user_esp;
    uint32_t user_eip;

    uint32_t kernel_esp_sch;
    uint32_t kernel_ebp_sch;
    uint32_t kernel_esp_exc;
    uint32_t kernel_ebp_exc;

    // fileds for rtc
    int rtc_opened; //indicate whether the rtc file is opened in this process
    int virtual_freq;          
    volatile int current_count; // when the current count reach zero, we indicate a virtual irq  （later for file position field of fd）
    volatile int virtual_iqr_got; // gotten a virtual iqr
} pcb;

fop_t rtc_fop_t;
fop_t dir_fop_t;
fop_t reg_fop_t;
fop_t stdi_fop_t;
fop_t stdo_fop_t;

/* open a file */
int32_t open(const uint8_t* fname);

/* close a file descriptor */
int32_t close(int32_t fd);

/* read data from keyboard, a file, device, or directory */
int32_t read(int32_t fd, void* buf, int32_t nbytes);

/* writes data to terminal or a device */
int32_t write(int32_t fd, const void* buf, int32_t nbytes);

/* copy program args from kernel to user */
int32_t getargs (uint8_t* buf, int32_t nbytes);

/* envoke a user program */
int32_t execute(const uint8_t* command);

/* Checkpoint 3.4 task */
int32_t vidmap (uint8_t** screen_start);

/* Signal Support for extra credit, just fake placeholder now */
int32_t set_handler (int32_t signum, void* handler_address);
int32_t sigreturn (void);

/* =============================================================================== */
/* bad calls */
int32_t badread(int32_t fd, void* buf, int32_t nbytes);
int32_t badwrite(int32_t fd, const void* buf, int32_t nbytes);

/* initialize the file operations table poinetr */
void fop_t_init();

/* Halt function for exception only */
void exp_halt();

/* Helper function for execute and halt */
int32_t _parse_cmd_(const uint8_t* command, uint8_t* filename, uint8_t* args);
int32_t _file_validation_(const uint8_t* filename);
int32_t _mem_setting_(const uint8_t* filename, int32_t* eip);
int32_t _PCB_setting_(const uint8_t* filename, const uint8_t* args, int32_t* eip);
void _fd_init_(pcb* pcb_addr);
void _context_switch_();
pcb* get_pcb_ptr(int32_t pid);


#endif
