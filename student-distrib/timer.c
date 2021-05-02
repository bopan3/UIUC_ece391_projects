#include "timer.h"
#include "scheduler.h"

/* 
 * pic_init
 *   DESCRIPTION: initialize the pit as timer chip for multi-task scheduler system
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS:  initialize the pit, which will raise highest INT to PIC at period within 10ms - 50ms
 */
void pit_init(){
    /* Set the mode register */
    outb(MODE_CTL_WORD, MODE_REG);

    /* Set INT Frequency */
    // outw(FRE_DIVS, CH0_D_PORT);
    outb(FRE_DIVS & 0xFF, CH0_D_PORT);
    outb((FRE_DIVS & 0xFF)>>8, CH0_D_PORT);
    /* enable INT */
    enable_irq((uint32_t) PIT_IRQ);

    return ;
}

/* 
 * pic_init
 *   DESCRIPTION: initialize the pit as timer chip for multi-task scheduler system
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS:  initialize the pit, which will raise highest INT to PIC at period within 10ms - 50ms
 */
void pit_handler(){
    cli();
    send_eoi(PIT_IRQ);
    if (ENABLE_SCHE){
        scheduler();
    }
    sti();
    return ;
}
