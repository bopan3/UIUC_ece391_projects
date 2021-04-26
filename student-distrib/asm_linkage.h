/*
 * Header file for 
 */

#ifndef ASM_LINKAGE_H
#define ASM_LINKAGE_H

/* Functions generated */
extern void irq_NMI_Interrupt();
extern void irq_Timer_Chip();
extern void irq_Keyboard();
extern void irq_Serial_Port();
extern void irq_Real_Time_Clock();
extern void irq_Eth0();
extern void irq_PS2_Mouse();
extern void irq_Ide0();
extern void asm_sys_linkage();
extern void page_excpt_asmlink();

#endif
