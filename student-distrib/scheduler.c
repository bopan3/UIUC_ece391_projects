#include "scheduler.h"

extern int32_t pid;             /* in sys_call.c */
terminal_t tm_array[MAX_TM];
int32_t terminal_tick = -1;      /* for the active running terminal, default the first terminal */
int32_t terminal_display = 0;   /* for the displayed terminal, only change when function-key pressed */
int32_t processor_id_act = DEFAULT_PROC;

void scheduler_init(){
    int i;      /* loop index */

    /* initialized the terminal array */
    for (i = 0; i < MAX_TM; i++){
        tm_array[i].tm_pid = TM_UNUSED;
        tm_array[i].kb_buf[0] = '\0'; /* TODO: not sure */
        tm_array[i].num_char = 0;
    }

    tm_array[0].dis_addr = (uint32_t*)TERMINAL_1_ADDR;
    tm_array[1].dis_addr = (uint32_t*)TERMINAL_2_ADDR;
    tm_array[2].dis_addr = (uint32_t*)TERMINAL_3_ADDR;
}

void scheduler(){
    int32_t kernel_esp;

    /* backup current pcb */
    pcb* cur_pcb_ptr = get_pcb_ptr(pid);
    asm volatile ( "movl %%esp, %0" : "=r"(kernel_esp) );
    cur_pcb_ptr->kernel_esp = kernel_esp;

    /* switch terminal */
    // tm_array[terminal_tick].tm_pid = pid;

    terminal_tick = (terminal_tick + 1) % MAX_TM;
    pid = tm_array[terminal_tick].tm_pid;


    /* switch terminal for task switch */
    _schedule_switch_tm_();





    return ;
}

/* helper function */
void _schedule_switch_tm_(){
    pcb* cur_pcb;

    /* default to create a shell for each terminal */
    if (tm_array[terminal_tick].tm_pid == TM_UNUSED){
        execute((uint8_t*)"shell");
    }
    else {
        cur_pcb = get_pcb_ptr(pid);
        
        /* restores next process's TSS */
        tss.ss0 = KERNEL_DS;
        tss.esp0 = _8MB_ - (_8KB_ * pid) - 4;

        /* paging setting */
        /* set user program address */
        page_dict[USER_PROG_ADDR].bit31_22 = pid + 2; /* start from 8MB */

        /* set video memory map */
        page_table[VIDEO_REGION_START_K].address = VIDEO_REGION_START_K +  (terminal_display != terminal_tick) * (terminal_tick + 1); /* set for kernel */
        page_table_vedio_mem[VIDEO_REGION_START_U].address =  VIDEO_REGION_START_K + (terminal_display != terminal_tick) * (terminal_tick + 1); /* set for user */

        TLB_flush();

        /* switch ESP & EBP */
        asm volatile ("movl %0, %%ebp": : "r"(cur_pcb->kernel_ebp));
        asm volatile ("movl %0, %%esp": : "r"(cur_pcb->kernel_esp));
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
    uint32_t* VM_addr = (uint32_t*)(VIDEO);
    int32_t i;

    /* check if switch to the current terminal */
    if (new_tm_id == terminal_display) return ;

    /* Save old terminal's screen to video page assigned for it
       restore new terminal's screen to video memory */
    for (i = 0; i < _4KB_; i++) {
        old_dis_addr[i] = VM_addr[i];
        VM_addr[i] = new_dis_addr[i];
    }

    terminal_tick = new_tm_id;   
}
