#include "scheduler.h"

extern int32_t pid;             /* in sys_call.c */
int32_t running_terminal = 1; 
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
        tm_array[i].num_char = 0;
        tm_array[i].x = 0;
        tm_array[i].y = 0; 
    }

    // tm_array[0].dis_addr = (uint8_t*)TERMINAL_1_ADDR;
    // tm_array[1].dis_addr = (uint8_t*)TERMINAL_2_ADDR;
    // tm_array[2].dis_addr = (uint8_t*)TERMINAL_3_ADDR;
}

void scheduler(){
    pcb* cur_pcb;
    pcb* old_pcb;
    old_pcb = get_pcb_ptr(pid);
    int32_t k_ebp, k_esp;

     /* backup current pcb */
    asm volatile (
        "movl %%ebp, %%eax;"
        "movl %%esp, %%ebx;"
        : "=a"(k_ebp), "=b"(k_esp)
        : /* no inputs */
    );
    old_pcb->kernel_ebp_sch = k_ebp;
    old_pcb->kernel_esp_sch = k_esp;

    /* switch terminal */
    terminal_tick = (terminal_tick + 1) % MAX_TM;

    /* default to create a shell for each terminal */
    if (tm_array[terminal_tick].tm_pid == TM_UNUSED){
        running_terminal ++;
        execute((uint8_t*)"shell");
    } else {
        
        pid = tm_array[terminal_tick].tm_pid;
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
        k_ebp = cur_pcb->kernel_ebp_sch;
        k_esp = cur_pcb->kernel_esp_sch;

        /* restore next task pcb */
        asm volatile (
            "movl %%eax, %%ebp;"
            "movl %%ebx, %%esp;"
            :   /* no outputs */
            : "a"(k_ebp), "b"(k_esp)
            : "ebp", "esp"
        );

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
    uint8_t* VM_addr = (uint8_t*)(VIDEO);                       /* physical displayed video memory base */
    int32_t i;                                                  /* loop index */

    /* check if switch to the current terminal */
    if (new_tm_id == terminal_display) {
        return ;
    }

    page_table[VIDEO_REGION_START_K].address = VIDEO_REGION_START_K;
    /* Save old terminal's screen to video page assigned for it
       restore new terminal's screen to video memory */
   for (i = 0; i < _4KB_; i++) {
       (VM_addr + _4KB_ * (terminal_display+ 1))[i] = VM_addr[i];
   }
   
    terminal_display = new_tm_id;

    for (i = 0; i < _4KB_; i++) {
       VM_addr[i] = (VM_addr + _4KB_ * (terminal_display+ 1))[i];
   }
    /* set video memory map */
    page_table[VIDEO_REGION_START_K].address = VIDEO_REGION_START_K +  (terminal_display != terminal_tick) * (terminal_tick + 1); /* set for kernel */
    page_table_vedio_mem[VIDEO_REGION_START_U].address =  VIDEO_REGION_START_K + (terminal_display != terminal_tick) * (terminal_tick + 1); /* set for user */
    // printf("Switch to terminal %d", terminal_display);

    update_cursor();
     
    return ;
}
