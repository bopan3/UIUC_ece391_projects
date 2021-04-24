#include "sys_calls.h"
#include "terminal.h"
#include "x86_desc.h"

#define MAX_TM 3
#define PROCESSOR_NOT_USED 0
#define PROCESSOR_USED 1
#define DEFAULT_PROC 0 
#define TM_UNUSED -1
#define VIDEO_REGION_START_K (VIDEO / _4KB_)
#define VIDEO_REGION_START_U 0

typedef struct terminal_t{
    int32_t tm_pid; /* trick the running program */
    char kb_buf[LINE_BUF_SIZE];
    int num_char = 0;

    /* For terminal display switch */
    uint32_t* dis_addr;      // address of video memory
}terminal_t;

void scheduler_init();
void scheduler();
void switch_visible_terminal(int new_tm_id);
