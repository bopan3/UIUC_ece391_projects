#include "scheduler.h"
extern int32_t pid;         /* in sys_call.c */
terminal_t tm_array[MAX_TM]; 
int32_t terminal_tick = 0;  /* for the active running terminal, default the first terminal */
int32_t terminal_display = 0; /* for the displayed terminal, only change when function-key pressed */
int32_t processor_id_act = DEFAULT_PROC;

void scheduler_init(){
    int i;      /* loop index */

    /* initialized the terminal array */
    for (i = 0; i < MAX_TM; i++){
        tm_array[i].tm_pid = TM_UNUSED;
        tm_array[i].kb_buf[0] = '\0'; /* TODO: not sure */
    }
}

void scheduler(){
    int32_t kernel_esp;

    /* backup current pcb */
    pcb* cur_pcb_ptr = get_pcb_ptr(pid);
    asm volatile ( "movl %%esp, %0" : "=r"(kernel_esp) );
    cur_pcb_ptr->kernel_esp = kernel_esp;

    /* switch terminal */
    tm_array[terminal_tick].tm_pid = pid;

    terminal_tick = (terminal_tick + 1)%MAX_TM;
    pid = tm_array[terminal_tick].tm_pid;

    /*  */


    return ;
}


void _schedule_switch_tm_(){
    if (tm_array[terminal_tick].tm_pid == TM_UNUSED){
        execute("shell");
    }
    tss.esp0 = ; /* TODO */
    /* TODO: paging setting */

    return ;
}
