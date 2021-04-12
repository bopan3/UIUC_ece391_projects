#ifndef IDT_H
#define IDT_H

#define IDT_SIZE 256
#define EXCP_NUM 20
#define SYS_CALL_ID 0x80

#define RETURN_FROM_EXP 0xFF    // indicating halt system call that it is called by an exception handler

#ifndef ASM
    void idt_init();
#endif

#endif
