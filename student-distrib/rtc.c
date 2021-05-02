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
#define MIN_FREQ 2
#define NULL 0

extern int32_t pid;
extern int8_t task_array[MAX_PROC];
static void rtc_byte_write(int8_t rtc_register, int8_t rtc_data);
static int8_t rtc_byte_read(int8_t rtc_register);
static void rtc_set_real_freq_level(int8_t freq_level);

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
      rtc_set_real_freq_level(MAX_FREQ_LEVEL);
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
    int32_t cur_pid;
    pcb* cur_pcb_ptr;
    cli();
    for (cur_pid=0;cur_pid<MAX_PROC;cur_pid++){
        //check if the process for this pid is on
        if (task_array[cur_pid]==0){continue;}
        cur_pcb_ptr = get_pcb_ptr(cur_pid);
        if (cur_pcb_ptr->rtc_opened==1){
            cur_pcb_ptr->current_count=cur_pcb_ptr->current_count-cur_pcb_ptr->virtual_freq;
            if (cur_pcb_ptr->current_count==0){
                cur_pcb_ptr->virtual_iqr_got=1;
                cur_pcb_ptr->current_count=MAX_FREQ;
            }
        }  
    }
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
void rtc_byte_write(int8_t rtc_register,int8_t rtc_data){
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
int8_t rtc_byte_read(int8_t rtc_register){
      outb( rtc_register, RTC_IDX_PORT);		// select register
      return inb(RTC_DATA_PORT);	            // read data
}
/* 
 * rtc_set_real_freq_level
 *   DESCRIPTION: set the real frequency of rtc
 *   INPUTS: freq_level
 *   OUTPUTS: none
 *   RETURN VALUE: 
 *   SIDE EFFECTS:  the frequency of ryc will be  to 2^freq_level
 */
void rtc_set_real_freq_level(int8_t freq_level){
      int8_t prev;
      int8_t rate = 16-freq_level;              // 16 is the total num of rate levels(15) +1     The default value of rate is 0110, or 6.
      rate &= 0x0F;			    // rate must be above 2 and not over 15
      prev=rtc_byte_read(RTC_REG_A);
      rtc_byte_write(RTC_REG_A, (prev & 0xF0) | rate); //0xF0 clear the previous rate 
}

/* 
 * rtc_open
 *   DESCRIPTION: Open and initialize RTC file
 *   INPUTS: filename  - String filename
 *   OUTPUTS: none
 *   RETURN VALUE: 0
 *   SIDE EFFECTS: 
 */
int32_t rtc_open(const uint8_t* filename) {
    pcb* cur_pcb_ptr;
    cur_pcb_ptr = get_pcb_ptr(pid);

    cur_pcb_ptr->virtual_freq=2; // 2HZ is the defualt frequency
    cur_pcb_ptr->current_count=MAX_FREQ;
    cur_pcb_ptr->virtual_iqr_got=0;
    cur_pcb_ptr->rtc_opened=1;
    return 0;
}

/* 
 * rtc_close
 *   DESCRIPTION: close RTC file and reset the relate parameters
 *   INPUTS: filename  - String filename
 *   OUTPUTS: none
 *   RETURN VALUE: 0
 *   SIDE EFFECTS: 
 */
int32_t rtc_close(int32_t fd) {
    pcb* cur_pcb_ptr;
    cur_pcb_ptr = get_pcb_ptr(pid);
    cur_pcb_ptr->virtual_freq=0; // change the freq to 0 to indicate the rtc is not opened
    cur_pcb_ptr->current_count=MAX_FREQ;
    cur_pcb_ptr->virtual_iqr_got=0;
    cur_pcb_ptr->rtc_opened=0;
    return 0;
}

/*
 * rtc_read
 *   DESCRIPTION: wait for an virtual RTC irq
 *   INPUTS: fd (File descriptor number)
 *           buf  (Output data pointer)
 *           nbytes (Number of bytes read)
 *   OUTPUTS: none
 *   RETURN VALUE: 0
 *   SIDE EFFECTS:
 */
int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes) {
    pcb* cur_pcb_ptr;
    cli();
    cur_pcb_ptr = get_pcb_ptr(pid);
    cur_pcb_ptr->virtual_iqr_got=0;
    cur_pcb_ptr->current_count=MAX_FREQ;
    sti();
    while (cur_pcb_ptr->virtual_iqr_got==0){};
    return 0;
}

/*
 * rtc_write
 *   DESCRIPTION: wait for an virtual RTC irq
 *   INPUTS: fd (File descriptor number)
 *           buf  (Output data pointer)
 *           nbytes (Number of bytes to write)
 *   OUTPUTS: none
 *   RETURN VALUE: 0 on success, -1 on invalid freqency
 *   SIDE EFFECTS: change the virtual irq freq
 */
int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes) {
    pcb* cur_pcb_ptr;
    int try_freq; // the freqency we try to change to 
    try_freq=   *((int*) buf);
    cur_pcb_ptr = get_pcb_ptr(pid);
    // check NULL pointer and wrong nbytes
    if (buf==NULL || (nbytes != sizeof(int32_t)) ){
        return -1;
    }

    // Check if  try_freq is not powers or out of bound
    if( ((try_freq & (try_freq - 1)) != 0) ||  (try_freq > MAX_FREQ || try_freq < MIN_FREQ)   ) {
        return -1;
    }

    cli();
    cur_pcb_ptr->virtual_freq=try_freq;
    sti();
    return 0;
}
