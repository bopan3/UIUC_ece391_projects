#include "cmos.h"
#include "../lib.h"

uint16_t yyyy = 0;
uint8_t  mon = 0;
uint8_t  dd = 0;
uint8_t  hh = 0;
uint8_t  mm = 0;
uint8_t  ss = 0;
uint8_t  ww = 0;

const char* weekday[7] = {"Sun.", "Mon.", "Tue.", "Wed.", "Thurs", "Fri.", "Sat."};

/* The RTC keeps track of the date and time, even when the computer's power is off.  */
void update_time(){
    outb(YEAR_PROT, CMOS_ADDR);
    yyyy = HEX2DEC( inb(CMOS_DATA));

    outb(MONTH_PORT, CMOS_ADDR);
    mon = HEX2DEC(inb(CMOS_DATA));

    outb(DAY_PORT, CMOS_ADDR);
    dd = HEX2DEC(inb(CMOS_DATA));

    outb(WEEK_PORT, CMOS_ADDR);
    ww = inb(CMOS_DATA);

    outb(HOUR_PORT, CMOS_ADDR);
    hh = HEX2DEC(inb(CMOS_DATA));

    outb(MIN_PORT, CMOS_ADDR);
    mm = HEX2DEC(inb(CMOS_DATA));

    outb(SEC_PORT, CMOS_ADDR);
    ss = HEX2DEC(inb(CMOS_DATA));

    return ;

}

void display_time(){
    printf("20%d-%d-%d, %d:%d:%d, on Day %s, UTC", yyyy, mon, dd, hh,mm,ss, weekday[ww-1]);
}
