#include "tests.h"
#include "i8259.h"
#include "x86_desc.h"
#include "lib.h"

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


/* Checkpoint 1 tests */

/* IDT Test - Example
 * 
 * Asserts that first 10 IDT entries are not NULL
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */
int idt_test(){
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

// add more tests here

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

    printf("\n The Master Mask values are %x \n", inb(MASTER_8259_DATA));
    printf("The Slave Mask values are %x \n", inb(SLAVE_8259_DATA));

    disable_irq(0);
    disable_irq(2);
    disable_irq(4);

    disable_irq(9);
    disable_irq(11);
    disable_irq(12);

    printf("\n The Master Mask values are %x", inb(MASTER_8259_DATA));
    printf("The Slave Mask values are %x \n", inb(SLAVE_8259_DATA));

    enable_irq(0);
    enable_irq(2);
    enable_irq(4);

    enable_irq(9);
    enable_irq(11);
    enable_irq(12);

    printf("\n The Master Mask values are %x \n", inb(MASTER_8259_DATA));
    printf("The Slave Mask values are %x \n", inb(SLAVE_8259_DATA));

    return result;
}

/* Checkpoint 2 tests */
/* Checkpoint 3 tests */
/* Checkpoint 4 tests */
/* Checkpoint 5 tests */


/* Test suite entry point */
void launch_tests(){
	TEST_OUTPUT("idt_test", idt_test());
	// launch your tests here
    TEST_OUTPUT("pic_test", pic_test());
}
