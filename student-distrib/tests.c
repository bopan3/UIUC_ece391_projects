#include "tests.h"
#include "i8259.h"
#include "x86_desc.h"
#include "lib.h"
#include "handlers.h"

#define PASS 1
#define FAIL 0

/* format these macros as you see fit */
#define TEST_HEADER 	\
	printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
#define TEST_OUTPUT(name, result)	\
	printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL");

static inline void assertion_failure(){
	/* Use exception #15 for assertions, otherwise
	   reserved by Intel */
	asm volatile("int $15");
}


/*-------------------- Checkpoint 1 tests --------------------*/

/* 
 * Check point 1.1 (Initialize the IDT, Test 1) 
 * Coverage: Asserts that first 10 IDT entries are not NULL
 * Files: x86_desc.h/S
 * Edited by WNC
 */
int CP1_idt_test_1(){
	TEST_HEADER;

	int i;
	int result = PASS;
	for (i = 0; i < 10; ++i){
		if ((idt[i].offset_15_00 == NULL) && 
			(idt[i].offset_31_16 == NULL)){
			assertion_failure();
			result = FAIL;
		}
	}
	
	return result;
}

/* 
 * Check point 1.1 (Initialize the IDT, Test 2) 
 * Coverage: Raise exception/interrupt, print a prompt and freezing the screen; 
 * 			 multiple exceptions; suppress interrupt when exception is excuting
 * Files: idt.c
 * Edited by WNC
 */
int CP1_idt_test_2(){
	TEST_HEADER;

	/* Pleas unmark one to demo, see idt.c for specific names of them */
	/* To test freezing, unmask two exceptions and only the first one should cause a prompt */
	/* To test interrupt suppression, issue an exception and try to press the key, only exception prompt will appear */

	/* Exceptions */
	asm volatile("int $0");		// EXCP_Divide_Error
	// asm volatile("int $6");		// EXCP_Invalid_Opcode
	asm volatile("int $14");		// EXCP_Page_Fault
	// asm volatile("int $17");		// EXCP_Alignment_Check 
	// asm volatile("int $19");		// EXCP_SIMD_Floating_Point

	/* Interrupt */
	// asm volatile("int $2");		// IRQ_NMI_Interrupt
	// asm volatile("int $32");		// IRQ_Timer_Chip
	// asm volatile("int $40");		// IRQ_Real_Time_Clock
	// asm volatile("int $46");		// IRQ_Ide0
	// asm volatile("int $33");		// IRQ_Keyboard

	/* System call */
	// asm volatile("int $128");		// SYS_System_Call

	return PASS;
}

/* 
 * Check point 1.1 (Initialize the IDT, Test 3) 
 * Coverage: Dereference a NULL pointer to issue exception
 * Files: idt.c
 * Edited by WNC
 */
int CP1_idt_test_3(){
	TEST_HEADER;

	int* a = NULL;
	int b = 1;
	b = b + *a;

	return PASS;
}

/* PIC Test - Example
 *
 * Test whether the PIC enable / disable the interrupt mask as expected
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: i8259_init()
 * Files: i8259.h/S
 */
int pic_test(){
    TEST_HEADER;

    int result = PASS;

    printf("\nThe Master Mask values are %x \n", inb(MASTER_8259_DATA));
    printf("The Slave Mask values are %x \n", inb(SLAVE_8259_DATA));

    enable_irq(0);
    enable_irq(4);

    enable_irq(9);
    enable_irq(11);
    enable_irq(12);

    printf("\nThe Master Mask values are %x \n", inb(MASTER_8259_DATA));    // 1110 1010
    printf("The Slave Mask values are %x \n", inb(SLAVE_8259_DATA));        // 1110 0101

    if ((0xE8 != inb(MASTER_8259_DATA)) | (0xE4 != inb(SLAVE_8259_DATA)))
        result = FAIL;

    disable_irq(0);
    disable_irq(4);

    disable_irq(9);
    disable_irq(11);
    disable_irq(12);

    printf("\nThe Master Mask values are %x \n", inb(MASTER_8259_DATA));
    printf("The Slave Mask values are %x \n", inb(SLAVE_8259_DATA));

    return result;
}

/*-------------------- Checkpoint 2 tests --------------------*/
/*-------------------- Checkpoint 3 tests --------------------*/
/*-------------------- Checkpoint 4 tests --------------------*/
/*-------------------- Checkpoint 5 tests --------------------*/


/* Test suite entry point */
void launch_tests(){
	/* Check point 1 */
	TEST_OUTPUT("CP1_idt_test_1", CP1_idt_test_1());
	TEST_OUTPUT("CP1_idt_test_2", CP1_idt_test_2());
	// TEST_OUTPUT("CP1_idt_test_3", CP1_idt_test_3());
    // TEST_OUTPUT("pic_test", pic_test());
}
