/*
 *  Assembly linkage for interrupt and system call handlers
 */

#define ASM 1
#include "x86_desc.h"

/*-------------------- Interrupt --------------------*/

/* Define a micro that generate jump linkage for different interrupt */
/*
 * INPUT: vect - index in IDT
 * OUTPUT: none
 * RETURN: none
 * EFFECT: excute the interruption
 */
#define asm_irq_linkage(f_name, vect)        \
.globl f_name                               ;\
f_name:                                     ;\
    /* Save all register */                 ;\
    pushw   %fs                             ;\
    pushw   %es                             ;\
    pushw   %ds                             ;\
    pushl   %eax                            ;\
    pushl   %ebp                            ;\
    pushl   %edi                            ;\
    pushl   %esi                            ;\
    pushl   %edx                            ;\
    pushl   %ecx                            ;\
    pushl   %ebx                            ;\
    pushfl                                  ;\
    /* Push argument */                     ;\
    pushl   $vect                           ;\
    jmp     common_irq                      ;\


/* Call the handler and store registers */
common_irq:
    /* Call the hander */
    call    irq_handler
    /* Restore all register */
    popl    %eax
    popfl
    popl    %ebx
    popl    %ecx
    popl    %edx
    popl    %esi
    popl    %edi
    popl    %ebp
    popl    %eax
    popw    %ds
    popw    %es
    popw    %fs
    /* Return control */
    iret


/* Define linkage for each interrupt */
asm_irq_linkage(irq_NMI_Interrupt, IRQ_NMI_Interrupt);
asm_irq_linkage(irq_Timer_Chip, IRQ_Timer_Chip);
asm_irq_linkage(irq_Keyboard, IRQ_Keyboard);
asm_irq_linkage(irq_Serial_Port, IRQ_Serial_Port);
asm_irq_linkage(irq_Real_Time_Clock, IRQ_Real_Time_Clock);
asm_irq_linkage(irq_Eth0, IRQ_Eth0);
asm_irq_linkage(irq_PS2_Mouse, IRQ_PS2_Mouse);
asm_irq_linkage(irq_Ide0, IRQ_Ide0);

/*------------------- System call -------------------*/

// .globl asm_sys_linkage
