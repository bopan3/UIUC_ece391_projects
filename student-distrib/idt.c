/*
 * Initialization of IDT
 */

#include "idt.h"
#include "x86_desc.h"
#include "handlers.h"

/* 
 * jump tables for eahc exception/interrupt
 *   DESCRIPTION: pass number of exception/interrupt jump to the handler funtion
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: jump to the handler
 */
/* Exception jump functions, note that number 0-19 are exceptions' index in IDT */
static void excp_Divide_Error() {
    printf("Fault (#0): Divide Error\n");
    exp_handler(0);
    return;
}
static void excp_RESERVED() {
    printf("Fault (#1): RESERVED\n");
    exp_handler(1);
    return;
}
static void excp_NMI_Interrupt() {
    printf("Interrupt (#2): NMI Interrupt\n");
    irq_handler(2);
    return;
}
static void excp_Breakpoint() {
    printf("Trap (#3): Breakpoint\n");
    exp_handler(3);
    return;
}
static void excp_Overflow() {
    printf("Trap (#4): Overflow\n");
    exp_handler(4);
    return;
}
static void excp_BOUND_Range_Exceeded() {
    printf("Fault (#5): BOUND Range Exceeded\n");
    exp_handler(5);
    return;
}
static void excp_Invalid_Opcode() {
    printf("Fault (#6): Invalid Opcode\n");
    exp_handler(6);
    return;
}
static void excp_Device_Not_Available() {
    printf("Fault (#7): Device Not Available\n");
    exp_handler(7);
    return;
}
static void excp_Double_Fault() {
    printf("Abort (#8): Double Fault\n");
    exp_handler(8);
    return;
}
static void excp_Coprocessor_Segment_Overrun() {
    printf("Fault (#9): Coprocessor Segment Overrun\n");
    exp_handler(9);
    return;
}
static void excp_Invalid_TSS() {
    printf("Fault (#10): Invalid TSS\n");
    exp_handler(10);
    return;
}
static void excp_Segment_Not_Present() {
    printf("Fault (#11): Segment Not Present\n");
    exp_handler(11);
    return;
}
static void excp_Stack_Segment_Fault() {
    printf("Fault (#12): Stack-Segment Fault\n");
    exp_handler(12);
    return;
}
static void excp_General_Protection() {
    printf("Fault (#13): General Protection\n");
    exp_handler(13);
    return;
}
static void excp_Page_Fault() {
    printf("Fault (#14): Page Fault\n");
    exp_handler(14);
    return;
}
static void excp_FPU_Floating_Point() {
    printf("Fault (#16): x87 FPU Floating-Point Error\n");
    exp_handler(16);
    return;
}
static void excp_Alignment_Check() {
    printf("Fault (#17): Alignment Check\n");
    exp_handler(17);
    return;
}
static void excp_Machine_Check() {
    printf("Abort (#18): Machine Check\n");
    exp_handler(18);
    return;
}
static void excp_SIMD_Floating_Point() {
    printf("Fault (#19): SIMD Floating-Point Exception\n");
    exp_handler(19);
    return;
}

/* Interrupt jump functions, note that hex/decimal numbers are interrupts' index in IDT */
static void irq_Timer_Chip() {
    printf("Interrupt (#32): Timer Chip\n");
    irq_handler(0x20);
    return;
}
static void irq_Keyboard() {
    printf("Interrupt (#33): Keyboard\n");
    irq_handler(0x21);
    return;
}
static void irq_Serial_Port() {
    printf("Interrupt (#36): Serial Port\n");
    irq_handler(0x24);
    return;
}
static void irq_Real_Time_Clock() {
    printf("Interrupt (#40): Real Time Clock\n");
    irq_handler(0x28);
    return;
}
static void irq_Eth0() {
    printf("Interrupt (#43): Eth0\n");
    irq_handler(0x2B);
    return;
}
static void irq_PS2_Mouse() {
    printf("Interrupt (#44): PS/2 Mouse\n");
    irq_handler(0x2C);
    return;
}
static void irq_Ide0() {
    printf("Interrupt (#46): Ide0\n");
    irq_handler(0x2E);
    return;
}


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
    SET_IDT_ENTRY(idt[1],excp_RESERVED );
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

    /* Set interrupt handlers manually */
    SET_IDT_ENTRY(idt[32], irq_Timer_Chip);
    SET_IDT_ENTRY(idt[33], irq_Keyboard);
    SET_IDT_ENTRY(idt[36], irq_Serial_Port);
    SET_IDT_ENTRY(idt[40], irq_Real_Time_Clock);
    SET_IDT_ENTRY(idt[43], irq_Eth0);
    SET_IDT_ENTRY(idt[44], irq_PS2_Mouse);
    SET_IDT_ENTRY(idt[46], irq_Ide0);
}



