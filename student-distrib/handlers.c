/*
 * Two handler functinos for exceptions and interrupt
 * 
 */

#include "handlers.h"
#include "lib.h"

/* 
 * exp_handler
 *   DESCRIPTION: save registers and pass control to a exception handler specified by exp_num
 *   INPUTS: exp_num - index of exception in IDT
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: excute exception handler
 */
void exp_handler(int exp_num) {
    while(1){}
    return;
}

/* 
 * irq_handler
 *   DESCRIPTION: save registers and pass control to a interrupt handler specified by irq_num
 *   INPUTS: irq_num - index of interrupt in IDT
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: excute interrupt handler
 */
void irq_handler(int irq_num) {
    while(1){}
    return;
}
