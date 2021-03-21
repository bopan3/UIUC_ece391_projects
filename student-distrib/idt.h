#ifndef IDT_H
#define IDT_H

#define IDT_SIZE 256
#define EXCP_NUM 20
#define SYS_CALL_ID 0x80

#ifndef ASM
    void idt_init();
#endif

#endif
