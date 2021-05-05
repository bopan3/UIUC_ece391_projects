#include "../types.h"
/* reference:
    1. https://wiki.osdev.org/CMOS
    2. 

 */
/* data time format: yyyy-mm-dd-hh-mm-ss */
#define CMOS_ADDR 0x70
#define CMOS_DATA 0x71
#define HEX2DEC(x) ((x/16*10) +( x&0xF))



#define SEC_PORT 0x00
#define MIN_PORT 0x02
#define HOUR_PORT 0x04
#define WEEK_PORT 0x06

#define DAY_PORT  0x07
#define MONTH_PORT 0x08
#define YEAR_PROT 0x09
//  0x00      Seconds             0–59
//  0x02      Minutes             0–59
//  0x04      Hours               0–23 in 24-hour mode, 
//                                1–12 in 12-hour mode, highest bit set if pm
//  0x06      Weekday             1–7, Sunday = 1
//  0x07      Day of Month        1–31
//  0x08      Month               1–12
//  0x09      Year

void update_time();
