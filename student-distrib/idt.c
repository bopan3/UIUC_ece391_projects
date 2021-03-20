#include "idt.h"
#include "x86_desc.h"

extern void handler_excp_00
void idt_init(){
    int i;      /* loop index */

    /* Setting all IDT entry to initialized value */
    for (i = 0; i < IDT_SIZE; i++){
        idt[i].seg_selector = KERNEL_CS;            /* kernel code seg */
        idt[i].reserved4 = 0;                       /* 8 bits unused, set to 0 */

        /* Gatetype filed = [size:reserved123] */
        /* for 32-bit gates */
        /* INT Gate is 0xE = 1110 */
        /* Excp Gate is 0xF = 1111 */
        idt[i].reserved3 = (i < EXCP_NUM)? 1:0; 
        idt[i].reserved2 = 1;               
        idt[i].reserved1 = 1;
        idt[i].size = 1;                        

        idt[i].dpl = (i == SYS_CALL_ID)? 3:0;       /* System Call is 3, otherwise is 0 */
        idt[i].present = 0;                         /* all not used now */

        SET_IDT_ENTRY(idt[i] , NULL);               /* all handler not set yet */
    }

    /* Set Exception Part Handler manually */
    /* from IDT 0x00 to 0x0F */
    SET_IDT_ENTRY(idt[], )


}
