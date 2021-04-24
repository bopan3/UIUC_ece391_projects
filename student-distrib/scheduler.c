#include "scheduler.h"
#include "paging.h"
#include "sys_calls.h"
extern int32_t pid;             /* in sys_call.c */
terminal_t tm_array[MAX_TM]; 
int32_t terminal_tick = 0;      /* for the active running terminal, default the first terminal */
int32_t terminal_display = 0;   /* for the displayed terminal, only change when function-key pressed */
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

    terminal_tick = (terminal_tick + 1) % MAX_TM;
    pid = tm_array[terminal_tick].tm_pid;

    
    /* switch terminal for task switch */
    _schedule_switch_tm_();
    

    


    return ;
}

/* helper function */
void _schedule_switch_tm_(){
    /* default to create a shell for each terminal */
    if (tm_array[terminal_tick].tm_pid == TM_UNUSED){
        execute("shell");
    }
    else {
        /* restores next process's TSS */
        tss.ss0 = KERNEL_DS;
        tss.esp0 = _8MB_ - (_8KB_ * pid) - 4;

        /* paging setting */


        /* switch ESP & EBP */
        asm volatile ("movl %0, %%ebp": : "=r"());
        asm volatile ("movl %0, %%ebp": : "=r"());
    }
    
    return ;
}


/*
 *  switch_visible_terminal
 *   DESCRIPTION: switch the displayed terminal by function key 
 *   INPUTS: new_tm_id - id of new terminal
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: set memory and buffer state
 */
void switch_visible_terminal(int new_tm_id){
    uint32_t* old_dis_addr = tm_array[terminal_tick].dis_addr;
    uint32_t* new_dis_addr = tm_array[new_tm_id].dis_addr;
    uint32_t* VM_addr = (uint32_t*)(VIRTUAL_ADDR_VEDIO_PAGE);
    int32_t i;

    /* check if switch to the current terminal */
    if (new_tm_id == terminal_display) return ;

    /* Save old terminal's screen to video page assigned for it */
    for (i = 0; i < _4KB_; i++) {
        
    }

    /* Restore new terminal's screen to video memory */

    /* Set the new terminal's display page address to video memory page */

    /* Switch execution to new terminal's user program (???) */

    /* Switch the buffer information */


}