/*
 * Two handler functinos for exceptions and interrupt
 */

#include "handlers.h"
#include "lib.h"
#include "x86_desc.h"
#include "keyboard.h"

/* 
 * irq_handler
 *   DESCRIPTION: save registers and pass control to a interrupt handler specified by irq_vect
 *   INPUTS: irq_num - index of interrupt in IDT
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: excute interrupt handler
 */
void irq_handler(int irq_vect) {
    /* Enable interrupt */
    // asm volatile("sti");

    /* For CP1, just print the message */
    switch (irq_vect) {
        case IRQ_NMI_Interrupt:
            printf("INTERRUPT #0x%x: NMI Interrupt\n", irq_vect);
            break;
        case IRQ_Timer_Chip:
            printf("INTERRUPT #0x%x: Timer Chip\n", irq_vect);
            break;
        case IRQ_Keyboard:
            // printf("INTERRUPT #0x%x: Keyboard\n", irq_vect);
            keyboard_handler();
            break;
        case IRQ_Serial_Port :
            printf("INTERRUPT #0x%x: Serial Port\n", irq_vect);
            break;
        case IRQ_Real_Time_Clock:
            printf("INTERRUPT #0x%x: Real Time Clock\n", irq_vect);
            break;
        case IRQ_Eth0:
            printf("INTERRUPT #0x%x: Eth0\n", irq_vect);
            break;
        case IRQ_PS2_Mouse:
            printf("INTERRUPT #0x%x: PS/2 Mouse\n", irq_vect);
            break;
        case IRQ_Ide0:
            printf("INTERRUPT #0x%x: Ide0\n", irq_vect);
            break;
        default:
            printf("INTERRUPT #0x%x: not defined\n", irq_vect);
            break;
    }
    // while(1){}
    return;
}


/* 
 * sys_handler
 *   DESCRIPTION: save registers and pass control to a system call handler specified by exp_vect
 *   INPUTS: exp_num - index of exception in IDT
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: excute exception handler
 */
void sys_handler(int exp_vect) {
    while(1){}
    return;
}


