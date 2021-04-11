#define ASM 1
.globl Prepare

Prepare:
    andl    $0x00FF, %eax
    pushl   %eax
    pushl   %ebx
    pushfl  
    popl    %ebx
    orl     $0x200, %ebx
    pushl   %ebx
    pushl   %ecx
    pushl   %edx
    iret    

