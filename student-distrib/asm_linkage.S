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
call_table:
    .long 0x0   /* NULL, a placeholder, since call number is 1-index based */
    .long halt
    .long execute
    .long read
    .long write
    .long open 
    .long close
    .long getargs
    .long vidmap
    .long set_handler
    .long sigreturn



.globl asm_sys_linkage

asm_sys_linkage:
    /* save all regs value */
    /* EAX for call num and return value */
    /* EIP set by call */
    pushl %ebx
    pushl %ecx
    pushl %edx
    pushl %esi
    pushl %edi
    pushl %ebp
    pushl %esp
    pushfl 

    /* push args to kernel stack */
    pushl %edx  /* mode */
    pushl %ecx  /* flags */
    pushl %ebx  /* name */

    /* call */
    /* first check if call number valid, from 1~10 */
    cmpl $0, %eax 
    jle sys_invalid
    cmpl $10, %eax 
    jg  sys_invalid
    jmp sys_valid

sys_invalid:
    movl $-1, %eax  /* indicate an error */
    jmp sys_iret

sys_valid:
    /* via call number and call table */
    call *call_table(, %eax, 4)

sys_iret:
    /* pop args from kernel stack */
    addl $12, %esp

    /* restore all regs value */
    popfl 
    popl %esp
    popl %ebp
    popl %edi
    popl %esi 
    popl %edx
    popl %ecx
    popl %ebx

    /* return from kernel to user */
    iret

.globl page_excpt_asmlink                               
page_excpt_asmlink:                                     
    /* Save all register */                 
    // pushw   %fs                             
    // pushw   %es                             
    // pushw   %ds                             
    // pushl   %eax                            
    // pushl   %ebp                            
    // pushl   %edi                            
    // pushl   %esi                            
    // pushl   %edx                            
    // pushl   %ecx                            
    // pushl   %ebx                            
    // pushfl                                  
    /* Push argument */ 
    movl (%esp),%eax                    
    pushl   %eax       
    movl %cr2,%eax                    
    pushl   %eax       
    jmp     excpt_for_PF                      


/* Call the except for PF */
excpt_for_PF:
    /* Call the hander */
    call    excp_Page_Fault_in_C
    // @@ the code should never reach here @@
    /* Restore all register */
    // popl    %eax
    // popfl
    // popl    %ebx
    // popl    %ecx
    // popl    %edx
    // popl    %esi
    // popl    %edi
    // popl    %ebp
    // popl    %eax
    // popw    %ds
    // popw    %es
    // popw    %fs
    /* Return control */
    iret
