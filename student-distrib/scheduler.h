#include "sys_calls.h"
#include "terminal.h"
#include "x86_desc.h"
#include "paging.h"

#define MAX_TM 3
#define PROCESSOR_NOT_USED 0
#define PROCESSOR_USED 1
#define DEFAULT_PROC 0 
#define TM_UNUSED -1
#define VIDEO_REGION_START_K (VIDEO / _4KB_)
#define VIDEO_REGION_START_U 0

#define TERMINAL_1_ADDR (VIDEO + 1 * _4KB_)
#define TERMINAL_2_ADDR (VIDEO + 2 * _4KB_)
#define TERMINAL_3_ADDR (VIDEO + 3 * _4KB_)

typedef struct terminal_t{
    int32_t tm_pid; /* trick the running program */
    char kb_buf[LINE_BUF_SIZE];
    int num_char;

    /* For terminal display switch */
    uint32_t* dis_addr;      // address of video memory
}terminal_t;

void scheduler_init();
void scheduler();
void switch_visible_terminal(int new_tm_id);
void _schedule_switch_tm_();
