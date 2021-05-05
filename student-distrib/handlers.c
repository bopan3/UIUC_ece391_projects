/*
 * Two handler functinos for exceptions and interrupt
 */
#include "tests.h"
#include "handlers.h"
#include "lib.h"
#include "x86_desc.h"
#include "keyboard.h"
#include "rtc.h"
#include "timer.h"
#include "mouse.h"
#include "./dev/sound.h"

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

    // printf("(Test) IRQ #: %x\n", irq_vect);


    /* For CP1, just print the message */
    cli();
    switch (irq_vect) {
        case IRQ_NMI_Interrupt:
            printf("INTERRUPT #0x%x: NMI Interrupt\n", irq_vect);
            break;
        case IRQ_Timer_Chip:
//            printf("INTERRUPT #0x%x: Timer Chip\n", irq_vect);
            pit_handler();
            break;
        case IRQ_Keyboard:
            // printf("INTERRUPT #0x%x: Keyboard\n", irq_vect);
            keyboard_handler();
            break;
        case IRQ_Serial_Port :
            printf("INTERRUPT #0x%x: Serial Port\n", irq_vect);
            break;
        case IRQ_Real_Time_Clock:
            //printf("INTERRUPT #0x%x: Real Time Clock\n", irq_vect);
            #if TEST_RTC==1
            rtc_handler();
            #endif
            break;
        case IRQ_Eth0:
            printf("INTERRUPT #0x%x: Eth0\n", irq_vect);
            break;
        case IRQ_PS2_Mouse:
            mouse_irq_handler();
            break;
        case IRQ_Ide0:
            printf("INTERRUPT #0x%x: Ide0\n", irq_vect);
            break;
        case IRQ_sb16:
            // printf("INTERRUPT #0x%x: SB16\n", irq_vect);
            sb16_handler();
            break;
        default:
            printf("INTERRUPT #0x%x: not defined\n", irq_vect);
            break;
    }
    sti();
    return;
}
