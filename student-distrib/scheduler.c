#include "scheduler.h"
#include "paging.h"

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
    pcb* cur_pcb = get_pcb_ptr(pid);

    /* default to create a shell for each terminal */
    if (tm_array[terminal_tick].tm_pid == TM_UNUSED){
        execute("shell");
    }
    else {
        /* restores next process's TSS */
        tss.ss0 = KERNEL_DS;
        tss.esp0 = _8MB_ - (_8KB_ * pid) - 4;

        /* paging setting */
        /* set user program address */
        page_dict[USER_PROG_ADDR].bit31_22 = pid + 2; /* start from 8MB */ 
        
        /* set video memory map */
        page_table[VIDEO_REGION_START].address = VIDEO_REGION_START +  (terminal_display != terminal_tick) * (terminal_tick + 1); /* set for kernel */
        page_table_vedio_mem[VIDEO_REGION_START_U].address =  VIDEO_REGION_START + (terminal_display != terminal_tick) * (terminal_tick + 1); /* set for user */

        TLB_flush();

        /* switch ESP & EBP */
        asm volatile ("movl %0, %%ebp": : "=r"(cur_pcb->kernel_ebp));
        asm volatile ("movl %0, %%esp": : "=r"(cur_pcb->kernel_esp));
    }
    
    return ;
}


/* switch the displayed terminal by function key 
 */
void switch_visible_terminal(int new_tm_id){
    /* check if switch to the current terminal */
    if (new_tm_id == terminal_display) return ;

    /* switch to background terminal */

}