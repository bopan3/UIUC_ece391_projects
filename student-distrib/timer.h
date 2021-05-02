#include "lib.h"
#include "i8259.h"
/* Refer to https://wiki.osdev.org/PIT, http://www.osdever.net/bkerndev/Docs/pit.htm */
/* 
I/O port     Usage
0x40         Channel 0 data port (read/write)
0x43         Mode/Command register (write only, a read is ignored)

Mode/Command register at I/O address 0x43 
Bits         Usage
6 and 7      Select channel :
                0 0 = Channel 0
                0 1 = Channel 1
                1 0 = Channel 2
                1 1 = Read-back command (8254 only)
4 and 5      Access mode :
                0 0 = Latch count value command
                0 1 = Access mode: lobyte only
                1 0 = Access mode: hibyte only
                1 1 = Access mode: lobyte/hibyte
1 to 3       Operating mode :
                0 0 0 = Mode 0 (interrupt on terminal count)
                0 0 1 = Mode 1 (hardware re-triggerable one-shot)
                0 1 0 = Mode 2 (rate generator)
                0 1 1 = Mode 3 (square wave generator)
                1 0 0 = Mode 4 (software triggered strobe)
                1 0 1 = Mode 5 (hardware triggered strobe)
                1 1 0 = Mode 2 (rate generator, same as 010b)
                1 1 1 = Mode 3 (square wave generator, same as 011b)
0            BCD/Binary mode: 0 = 16-bit binary, 1 = four-digit BCD
*/
#define MODE_REG 0x43
#define CH0_D_PORT 0x40 /* for channel 0 */

/* Mode control word  
* b67: 00 for chanel 0
* b45: 11 for R/W both bytes
* b1-3: mode 3 for accurate interupt frequency
* b0: 16-bit binary
*/
#define MODE_CTL_WORD  0x36
#define FRE_DIVD 1193180 /* from doc, in hz */
#define EXP_TIME    15   /* ms */
#define FRE_DIVS (FRE_DIVD / 1000 * EXP_TIME)
#define PIT_IRQ 0x00


void pit_init();
void pit_handler();
