#include "rtc.h"
#include "i8259.h"
#include "lib.h"
#define IRQ_NUM_RTC 0x08
#define RTC_IDX_PORT 0x70
#define RTC_DATA_PORT 0x71
// all register handling codes for RTC are adapt from https://wiki.osdev.org/RTC
/* 
 * rtc_init
 *   DESCRIPTION: initialize the rtc
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS:  initialize the rtc
 */
void rtc_init(){
      char prev;
      cli();
      // here 0x80 is for disable NMI, 0x0B is to specify we want  select register B,
      outb( 0x8B, RTC_IDX_PORT);		// select register B, and disable NMI
      prev=inb(RTC_DATA_PORT);	           // read the current value of register B
      outb( 0x0B, RTC_IDX_PORT);		// set the index again (a read will reset the index to register D)
      outb( prev | 0x40, RTC_DATA_PORT);	// write the previous value ORed with 0x40. This turns on bit 6 of register B
      //This will turn on the IRQ with the default 1024 Hz rate. 
      enable_irq(IRQ_NUM_RTC);
      outb(0x0C,RTC_IDX_PORT);	// select register C
      inb(RTC_DATA_PORT);		// just throw away contents
      sti();
}

/* 
 * rtc_handler
 *   DESCRIPTION: IRQ handler for rtc
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS:  call test_interrupts() to refresh the screen
 */
void rtc_handler(){
    cli();
    test_interrupts();
    outb(0x0C,RTC_IDX_PORT);	// select register C
    inb(RTC_DATA_PORT);		   // just throw away contents
    send_eoi(IRQ_NUM_RTC);
    sti();
}

