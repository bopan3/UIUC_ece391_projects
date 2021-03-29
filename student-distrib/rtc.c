#include "rtc.h"
#include "i8259.h"
#include "lib.h"
#define IRQ_NUM_RTC 0x08
#define RTC_IDX_PORT 0x70
#define RTC_DATA_PORT 0x71
#define RTC_REG_A 0x8A  // here 0x80 is for disable NMI, 0x0A is to specify we want  select register A
#define RTC_REG_B 0x8B  // here 0x80 is for disable NMI, 0x0B is to specify we want  select register B
#define RTC_REG_C 0x0C  // here 0x00 is for enable  NMI, 0x0C is to specify we want  select register C
#define BIT_6 0x40
#define MAX_FREQ_LEVEL 10  // the maximun frequency is 2^10
#define MIN_FREQ_LEVEL 1   // the minimun frequency is 2^1

static void rtc_byte_write(rtc_register,rtc_data);
static int8_t rtc_byte_read(rtc_register);
static int8_t rtc_set_freq_level(freq_level);

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
      int8_t prev;
      cli();
      //This will turn on the IRQ with the default 1024 Hz rate. 
      prev=rtc_byte_read(RTC_REG_B);  // read the current value of register B
      rtc_byte_write(RTC_REG_B,  prev | BIT_6 ); //  This turns on bit 6 of register B
      //unmask pic for rtc
      enable_irq(IRQ_NUM_RTC);
      //a read on RTC_REG_C to allow next irq
      rtc_byte_read(RTC_REG_C);
      rtc_set_freq_level(1);
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
    //a read on RTC_REG_C to allow next irq
    rtc_byte_read(RTC_REG_C);
    send_eoi(IRQ_NUM_RTC);
    sti();
}

/* 
 * rtc_byte_write
 *   DESCRIPTION: write "rtc_data" to the "rtc_register"
 *   INPUTS: rtc_data, rtc_register
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS:  data written to the rtc_register
 */
void rtc_byte_write(rtc_register,rtc_data){
      outb( rtc_register, RTC_IDX_PORT);		// select register
      outb( rtc_data    , RTC_DATA_PORT);	    // write data
}

/* 
 * rtc_byte_read
 *   DESCRIPTION: read from the "rtc_register" and return the "rtc_data" in it
 *   INPUTS: rtc_register
 *   OUTPUTS: rtc_data
 *   RETURN VALUE: data read from the rtc_register
 *   SIDE EFFECTS:  
 */
int8_t rtc_byte_read(rtc_register){
      outb( rtc_register, RTC_IDX_PORT);		// select register
      return inb(RTC_DATA_PORT);	            // read data
}
/* 
 * rtc_set_freq_level
 *   DESCRIPTION: set the frequency of rtc
 *   INPUTS: freq_level
 *   OUTPUTS: none
 *   RETURN VALUE: 
 *   SIDE EFFECTS:  the frequency of ryc will be  to 2^freq_level
 */
int8_t rtc_set_freq_level(freq_level){
      int8_t prev;
      int8_t rate = 16-freq_level;              // 16 is the total num of rate levels(15) +1     The default value of rate is 0110, or 6.
      rate &= 0x0F;			    // rate must be above 2 and not over 15
      cli();
      prev=rtc_byte_read(RTC_REG_A);
      rtc_byte_write(RTC_REG_A, (prev & 0xF0) | rate); //0xF0 clear the previous rate 
      sti();
}


