/*
 * Initialization of IDT
 */

#include "lib.h"

#include "idt.h"
#include "x86_desc.h"

/* 
 * Jump frunction for each exception/interrupt/system call
 *   DESCRIPTION: pass number of exception/interrupt jump to the handler funtion
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: jump to the handler
 */

#define IDT_exp_entry(f_name, vect, str)      \
void f_name() {                               \
    /* Suppress all interrupts */             \
    asm volatile("cli");                      \
    printf("EXCEPTION #0x%x: %s\n", vect, str); \
    while(1){}                                \
    asm volatile("sti");                      \
}                                             \

#define IDT_irq_entry(f_name, vect, str)      \
void f_name() {                               \
    printf("INTERRUPT #0x%x: %s\n", vect, str); \
    while(1){}                                \
}                                             \

#define IDT_sys_entry(f_name, vect, str)      \
void f_name() {                               \
    printf("SYSTEM CALL #0x%x: %s\n", vect, str);\
    while(1){}                                \
}                                             \

/* Exceptions */
IDT_exp_entry(excp_Divide_Error, 0, "Divide Error");
IDT_exp_entry(excp_RESERVED, 1, "RESERVED");
IDT_exp_entry(excp_Breakpoint, 3, "Breakpoint");
IDT_exp_entry(excp_Overflow, 4, "Overflow");
IDT_exp_entry(excp_BOUND_Range_Exceeded, 5, "BOUND Range Exceeded");
IDT_exp_entry(excp_Invalid_Opcode, 6, "Invalid Opcode");
IDT_exp_entry(excp_Device_Not_Available, 7, "Device Not Available");
IDT_exp_entry(excp_Double_Fault, 8, "Double Fault");
IDT_exp_entry(excp_Coprocessor_Segment_Overrun, 9, "Coprocessor Segment Overrun");
IDT_exp_entry(excp_Invalid_TSS, 10, "Invalid TSS");
IDT_exp_entry(excp_Segment_Not_Present, 11, "Segment Not Present");
IDT_exp_entry(excp_Stack_Segment_Fault, 12, "Stack-Segment Fault");
IDT_exp_entry(excp_General_Protection, 13, "General Protection");
IDT_exp_entry(excp_Page_Fault, 14, "Page Fault");
IDT_exp_entry(excp_FPU_Floating_Point, 16, "x87 FPU Floating-Point Error");
IDT_exp_entry(excp_Alignment_Check, 17, "Alignment Check");
IDT_exp_entry(excp_Machine_Check, 18, "Machine Check");
IDT_exp_entry(excp_SIMD_Floating_Point, 19, "SIMD Floating-Point Exception");

/* Interrupts */
IDT_irq_entry(excp_NMI_Interrupt, 2, "NMI Interrupt");
IDT_irq_entry(irq_Timer_Chip, 32, "Timer Chip");
IDT_irq_entry(irq_Keyboard, 33, "Keyboard");
IDT_irq_entry(irq_Serial_Port, 36, "Serial Port");
IDT_irq_entry(irq_Real_Time_Clock, 40, "Real Time Clock");
IDT_irq_entry(irq_Eth0, 43, "Eth0");
IDT_irq_entry(irq_PS2_Mouse, 44, "PS/2 Mouse");
IDT_irq_entry(irq_Ide0, 46, "Ide0");

/* System call */
IDT_sys_entry(System_Call, 128, "System call");


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
        idt[i].reserved3 = (i < EXCP_NUM)? 1:0; 
        idt[i].reserved2 = 1;               
        idt[i].reserved1 = 1;
        idt[i].size = 1;                        

        idt[i].dpl = (i == SYS_CALL_ID)? 3:0;       /* System Call is 3, otherwise is 0 */
        idt[i].present = 0;                         /* all not used now */

        SET_IDT_ENTRY(idt[i] , NULL);               /* all handler not set yet */
    }

    /* Set exception handlers manually (from IDT 0x00 to 0x0F) */
    SET_IDT_ENTRY(idt[0], excp_Divide_Error);
    SET_IDT_ENTRY(idt[1],excp_RESERVED);
    SET_IDT_ENTRY(idt[2], excp_NMI_Interrupt);
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
    SET_IDT_ENTRY(idt[14], excp_Page_Fault);
    // idt[15] reserved
    SET_IDT_ENTRY(idt[16], excp_FPU_Floating_Point);
    SET_IDT_ENTRY(idt[17], excp_Alignment_Check);
    SET_IDT_ENTRY(idt[18], excp_Machine_Check);
    SET_IDT_ENTRY(idt[19], excp_SIMD_Floating_Point);

    /* Set interrupt handlers manually (from 0x20 to 0x2F or 32 to 47) */
    SET_IDT_ENTRY(idt[32], irq_Timer_Chip);
    SET_IDT_ENTRY(idt[33], irq_Keyboard);
    SET_IDT_ENTRY(idt[36], irq_Serial_Port);
    SET_IDT_ENTRY(idt[40], irq_Real_Time_Clock);
    SET_IDT_ENTRY(idt[43], irq_Eth0);
    SET_IDT_ENTRY(idt[44], irq_PS2_Mouse);
    SET_IDT_ENTRY(idt[46], irq_Ide0);

    /* Set system call vector (0x80) */
    SET_IDT_ENTRY(idt[0x80], System_Call);
}



