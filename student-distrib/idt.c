/*
 * Initialization of IDT
 */

#include "lib.h"

#include "idt.h"
#include "x86_desc.h"
#include "asm_linkage.h"
#include "sys_calls.h"



/* 
 * Jump frunction for each exception/interrupt/system call
 *   DESCRIPTION: pass number of exception/interrupt jump to the handler funtion
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: jump to the handler
 */

#define IDT_exp_entry(f_name, vect, msg)      \
void f_name() {                               \
    /* Suppress all interrupts (just in case) */ \
    asm volatile("cli");                      \
    /* blue_screen(); */                      \
    printf("EXCEPTION #0x%x: %s\n", vect, msg); \
    /* while(1){} */                          \
    exp_halt();                    \
    asm volatile("sti");    /* should never reach here */ \
    return;                                   \
}                                             \

/*#define IDT_sys_entry(f_name, vect, msg)      \
//void f_name() {                               \
//    printf("SYSTEM CALL #0x%x: %s\n", vect, msg);\
//    asm volatile("sti");                      \
//    return;                                   \
//}                                             \
*/

/* Exceptions */
IDT_exp_entry(excp_Divide_Error, EXCP_Divide_Error, "Divide Error");
IDT_exp_entry(excp_RESERVED, EXCP_RESERVED, "RESERVED");
IDT_exp_entry(excp_Breakpoint, EXCP_Breakpoint, "Breakpoint");
IDT_exp_entry(excp_Overflow, EXCP_Overflow, "Overflow");
IDT_exp_entry(excp_BOUND_Range_Exceeded, EXCP_BOUND_Range_Exceeded, "BOUND Range Exceeded");
IDT_exp_entry(excp_Invalid_Opcode, EXCP_Invalid_Opcode, "Invalid Opcode");
IDT_exp_entry(excp_Device_Not_Available, EXCP_Device_Not_Available, "Device Not Available");
IDT_exp_entry(excp_Double_Fault, EXCP_Double_Fault, "Double Fault");
IDT_exp_entry(excp_Coprocessor_Segment_Overrun, EXCP_Coprocessor_Segment_Overrun, "Coprocessor Segment Overrun");
IDT_exp_entry(excp_Invalid_TSS, EXCP_Invalid_TSS, "Invalid TSS");
IDT_exp_entry(excp_Segment_Not_Present, EXCP_Segment_Not_Present, "Segment Not Present");
IDT_exp_entry(excp_Stack_Segment_Fault, EXCP_Stack_Segment_Fault, "Stack-Segment Fault");
IDT_exp_entry(excp_General_Protection, EXCP_General_Protection, "General Protection");
//IDT_exp_entry(excp_Page_Fault, EXCP_Page_Fault, "Page Fault");
void excp_Page_Fault_in_C(int32_t CR2, int32_t error_code, int32_t return_eip) {                               
    /* Suppress all interrupts (just in case) */ 
    asm volatile("cli");                      
    /* blue_screen(); */ 

    printf("===============================================================================\n");   
    printf("EXCEPTION #0x%x: %s\n", EXCP_Page_Fault, "Page Fault");
    printf("The address causing page fault: %x\n",CR2);
    printf("Error_code_PF: %d\n", error_code);
    printf("EIP: %d\n", return_eip);

    if ((error_code & 1) == 0) /*check the 0 bit for P*/ {printf("- The fault was caused by a non-present page\n");}
    else{printf("- The fault was caused when user try to access higher level page\n");}
    if ((error_code & 2) == 0) /*check the 1 bit for W/R*/ {printf("- The fault was caused by a read\n");}
    else{printf("- The fault was caused by a write\n");}
    if ((error_code & 4) == 0) /*check the 2 bit for U/S*/ {printf("- This happened when in kernal mode\n");}
    else{printf("- This happened when in user mode\n");}
    if ((error_code & 8) == 1) /*check the 3 bit for RSVD*/ {printf("- caused by reserved bit violation\n");}
    printf("===============================================================================\n");
    // while(1){}                          
    exp_halt();                    
    asm volatile("sti");    /* should never reach here */ 
    return;                                   
}  
IDT_exp_entry(excp_FPU_Floating_Point, EXCP_FPU_Floating_Point, "x87 FPU Floating-Point Error");
IDT_exp_entry(excp_Alignment_Check, EXCP_Alignment_Check, "Alignment Check");
IDT_exp_entry(excp_Machine_Check, EXCP_Machine_Check, "Machine Check");
IDT_exp_entry(excp_SIMD_Floating_Point, EXCP_SIMD_Floating_Point, "SIMD Floating-Point Exception");

/* System call */
//IDT_sys_entry(System_Call, SYS_System_Call, "System call");


/* 
 * idt_init
 *   DESCRIPTION: initialize the IDT table with descriptors
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: load the IDT table to memory specified by pointer idt
 */
void idt_init() {
    int i;      /* loop index */

    /* Setting all IDT entry to initialized value */
    for (i = 0; i < IDT_SIZE; i++) {
        idt[i].seg_selector = KERNEL_CS;            /* kernel code seg */
        idt[i].reserved4 = 0;                       /* 8 bits unused, set to 0 */

        /* Gatetype filed = [size:reserved123] */
        /* for 32-bit gates */
        /* INT Gate is 0xE = 1110 */
        /* Excp Gate is 0xF = 1111 */
        // idt[i].reserved3 = (i < EXCP_NUM)? 1:0; 
        idt[i].reserved3 = 0;
        idt[i].reserved2 = 1;               
        idt[i].reserved1 = 1;
        idt[i].size = 1;                        

        idt[i].dpl = (i == SYS_CALL_ID)? 3:0;       /* System Call is 3, otherwise is 0 */
        idt[i].present = 0;                         /* all not used now */

        SET_IDT_ENTRY(idt[i] , NULL);               /* all handler not set yet */
    }

    /* Set exception handlers manually (from IDT 0x00 to 0x0F) */
    SET_IDT_ENTRY(idt[0], excp_Divide_Error);
    SET_IDT_ENTRY(idt[1], excp_RESERVED);
    SET_IDT_ENTRY(idt[2], irq_NMI_Interrupt);
    SET_IDT_ENTRY(idt[3], excp_Breakpoint);
    SET_IDT_ENTRY(idt[4], excp_Overflow);
    SET_IDT_ENTRY(idt[5], excp_BOUND_Range_Exceeded);
    SET_IDT_ENTRY(idt[6], excp_Invalid_Opcode);
    SET_IDT_ENTRY(idt[7], excp_Device_Not_Available);
    SET_IDT_ENTRY(idt[8], excp_Double_Fault);
    SET_IDT_ENTRY(idt[9], excp_Coprocessor_Segment_Overrun);
    SET_IDT_ENTRY(idt[10], excp_Invalid_TSS);
    SET_IDT_ENTRY(idt[11], excp_Segment_Not_Present);
    SET_IDT_ENTRY(idt[12], excp_Stack_Segment_Fault);
    SET_IDT_ENTRY(idt[13], excp_General_Protection);
    SET_IDT_ENTRY(idt[14], page_excpt_asmlink);
    // idt[15] reserved
    SET_IDT_ENTRY(idt[16], excp_FPU_Floating_Point);
    SET_IDT_ENTRY(idt[17], excp_Alignment_Check);
    SET_IDT_ENTRY(idt[18], excp_Machine_Check);
    SET_IDT_ENTRY(idt[19], excp_SIMD_Floating_Point);

    /* Set interrupt handlers manually (from 0x20 to 0x2F or 32 to 47 decimal) */
    SET_IDT_ENTRY(idt[32], irq_Timer_Chip);
    SET_IDT_ENTRY(idt[33], irq_Keyboard);
    SET_IDT_ENTRY(idt[36], irq_Serial_Port);
    SET_IDT_ENTRY(idt[40], irq_Real_Time_Clock);
    SET_IDT_ENTRY(idt[43], irq_Eth0);
    SET_IDT_ENTRY(idt[44], irq_PS2_Mouse);
    SET_IDT_ENTRY(idt[46], irq_Ide0);

    /* Set system call vector (0x80) */
    SET_IDT_ENTRY(idt[0x80], asm_sys_linkage);
}



