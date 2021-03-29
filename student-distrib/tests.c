#include "tests.h"
#include "i8259.h"
#include "x86_desc.h"
#include "lib.h"
#include "handlers.h"
#include "paging.h"
#include "terminal.h"
#include "file_sys.h"

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

/* Check point 1.1 (Initialize the IDT, Test 1)
 * Coverage: Asserts that first 10 IDT entries are not NULL
 * Files: x86_desc.h/S
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

/* Check point 1.1 (Initialize the IDT, Test 2)
 * Coverage: Raise exception/interrupt, print a prompt and freezing the screen;
 * 			 multiple exceptions; suppress interrupt when exception is excuting
 * Files: idt.c
 */
int CP1_idt_test_2(){
	TEST_HEADER;

	/* Pleas unmark one to demo, see idt.c for specific names of them */
	/* To test freezing, unmask two exceptions and only the first one should cause a prompt */
	/* To test interrupt suppression, issue an exception and try to press the key, only exception prompt will appear */

	/* Exceptions */
	// asm volatile("int $0");		// EXCP_Divide_Error
	// asm volatile("int $6");		// EXCP_Invalid_Opcode
	// asm volatile("int $14");		// EXCP_Page_Fault
	// asm volatile("int $17");		// EXCP_Alignment_Check
	// asm volatile("int $19");		// EXCP_SIMD_Floating_Point

	/* Interrupt */
	// asm volatile("int $2");		// IRQ_NMI_Interrupt
	// asm volatile("int $32");		// IRQ_Timer_Chip
	// asm volatile("int $40");		// IRQ_Real_Time_Clock (may not have print message)
	// asm volatile("int $46");		// IRQ_Ide0
	// asm volatile("int $33");		// IRQ_Keyboard (may not have print message)

	/* System call */
	// asm volatile("int $128");		// SYS_System_Call

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


/* paging_test
 *
 * Test if the paging settings are valid and if all assigned page could work
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: paging_init()
 * Files: paging.c/h
 */
int paging_test(){
	TEST_HEADER;
	int i;								/* Loop index */
	int result = PASS;

	char* table_base = (char*)  VIDEO;	/* base address of video memory */
	char* dict_base = (char*) 0x400000;	/* base address of kernel page */
	char temp;							/* for r/w test */

	printf("=== Part 1: Paging Settings Validation ===\n");
	/* Check the settings of pages */
	/* If any fail, the test will fail */
	for (i = 0; i < PD_SIZE; i++){
		/* the first 2 4MB-chunks should be valid */
		if ((i==0 || i == 1) ^ (page_dict[i].P)){
			result = FAIL;
			return result;
		}
	}
	printf("===== Page Dict Setting Valid!\n");
	for (i = 0; i < PT_SIZE; i++){
		if (((i * _4KB_) == VIDEO) ^ page_table[i].P){
			/* only the video memory 4kb chunk is valid */
			result = FAIL;
			return result;
		}
	}
	printf("===== Page Table Setting Valid!\n");


	/* Do R/W test for all valid memory */
	/* If any fail, it will go to page fault excp */
	printf("=== Part 2: Dereference Functionality Validation ===\n");
	/* check all entry in video memory */
	for (i = 0; i < _4KB_; i++){
		temp = table_base[i];		/* test readability */
		table_base[i] = temp;		/* test writability */
	}
	printf("===== All Video Memory are R/W Enable!\n");

	/* check all entry in kernel page */
	for (i = 0; i < PD_SIZE * _4KB_; i++){
		temp = dict_base[i];
		dict_base[i] = temp;
	}
	printf("===== All kernel Page are R/W Enable!\n");
	return result;
}


/* deref
 *
 * wrapped R/W test on a given virtual address
 * Inputs: char* ptr - a virtual pointer
 * Outputs: None
 * Side Effects: to R/W operation
 */
void deref(char* ptr){
	char temp = *ptr;	/* try read */
	*ptr = temp;		/* try write */
	return ;
}



/* deref_safe
 *
 * wrapped R/W test with safety check on a given virtual address
 * Inputs: char* ptr - a virtual pointer
 * Outputs: PASS/FAIL
 * Side Effects: to R/W operation
 */
int deref_safe(char* ptr){
	int result = PASS;
	uint16_t pd_off = 0;		/* page dict offset */
	uint16_t pt_off = 0;		/* page table offset */
	char temp;
	PTE* pt_base = (PTE*) 0;	/* page dict base address */

	/* check NULL first */
	if (ptr == NULL){
		printf("===== The dereference pointer is NULL, will cause exception\n");
		result = FAIL;
		return result;
	}

	/* check pointer is valid */
	pd_off |= ((int) ptr & (0xFFC00000)) >> 22; /* get the 10 MSBs for page dict offset */
	pt_off |= ((int) ptr & (0x003FF000)) >> 12; /* get the middle 10 bits for page table offset */

	if (page_dict[pd_off].P != 1){
		printf("===== The accessed page dict entry is invalid\n===== pointer %x is invalid\n", ptr);
		result = FAIL;
		return result;
	}
	else {
		if (page_dict[pd_off].PS == 0){
			/* jump to the page table */
			pt_base = (PTE*)((page_dict[pd_off].bit31_22 << 22) | (page_dict[pd_off].bit21_13 << 13) | (page_dict[pd_off].bit12 << 12));
			if (pt_base[pt_off].P == 0){
				printf("===== accessed page table entry is invalid\n===== pointer %x is invalid\n", ptr);
				result = FAIL;
				return result;
			} else {
				/* R/W test */
				temp = *ptr;
				*ptr = temp;
				printf("===== R/W reference test at %x successfully\n", ptr);
				return result;
			}

		} else{
			/* 4 MB page dict entry, access directly */
			/* R/W test */
			temp = *ptr;
			*ptr = temp;
			printf("===== R/W reference test at %x successfully\n", ptr);
			return result;
		}

	}
}

/* paging_test
 *
 * Test if the paging works correctly, including invalid address will cause exception and valid address could work
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: paging_init()
 * Files: paging.c/h
 */
int paging_test_pf(){
	TEST_HEADER;

	int result = PASS;
	char* ptr;

	/* The following 3 tests would cause Page Fault as expected */

	// ptr = (char*) NULL;
	// printf("Dereference NULL Test\n");
	// deref(ptr);					/* should raise EXCP */

	// ptr = (char*) 0x3FFFFF;		/* the end of first chunk */
	// printf("Dereference first 4 MB chunk invalid address %x\n", ptr);
	// deref(ptr);

	// ptr = (char*) 0x800000;			/* the start of 3rd chunk */
	// printf("Dereference invalid chunk invalid address %x\n", ptr);
	// deref(ptr);


	/* The following 3 repeated test would not cause Page Fault, since the deref_safe() do address validation first */
	ptr = (char*) NULL;
	printf("Safe Dereference NULL Test\n");
	result = deref_safe(ptr);

	ptr = (char*) 0x3FFFFF;			/* the end of first chunk */
	printf("Safe Dereference first 4 MB chunk invalid address %x\n", ptr);
	result = deref_safe(ptr);

	ptr = (char*) 0x800000;			/* the start of 3rd chunk */
	printf("Safe Dereference invalid chunk invalid address %x\n", ptr);
	result = deref_safe(ptr);

	/* The following test work as expected */
	ptr = (char*) VIDEO;			/* the start of video memory */
	printf("Safe Dereference valid address %x\n", ptr);
	result = deref_safe(ptr);

	ptr = (char*) 0x400000;			/* the start of 2nd chunk */
	printf("Safe Dereference valid address %x\n", ptr);
	result = deref_safe(ptr);

	return result;	/* All safe test should pass, and any one of the unsafe test would cause Page Fault */
}

/*-------------------- Checkpoint 2 tests --------------------*/

/* term_read_write_test
 * Test if the read and write function of terminal works correctly
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: paging_init()
 * Files: paging.c/h
 */
int term_read_write_test() {
    int result = PASS;

    while (1) {
        char test_buf[128];
        printf("==========================Read & Write test starts...==========================\nPlease type anything and press enter:\n");
        terminal_read(0, test_buf, 128);
        printf("My turn:\n");
        terminal_write(0, test_buf, 128);
    }
    return result;
}


/* Check point 2.3 (File system)
 * Coverage: list all file information
 * Files: file_sys/c/h
 */
int cp2_filesys_test_1() {
	int32_t n, i, num_dentry;
	struct dentry_t result[30];
	char temp[33];

	printf("--------------------\n");

	// Read by index
	for (i = 0; i < 30; i++) {
		n = read_dentry_by_index((uint32_t)i, &result[i]);
		if (n == -1) {
			num_dentry = i;
			break;
		}
	}

	for (i = 0; i < num_dentry; i++) {
		strncpy((int8_t*)temp, (int8_t*)result[i].f_name, 32);
		temp[33] = '\0';
		printf("Name: %s  |  Type: %d  |  Size(byte): %d\n", temp, result[i].f_type, result[i].f_size);
	}
	
	return PASS;
}

/* Check point 2.3 (File system)
 * Coverage: read short files
 * Files: file_sys/c/h
 */
int cp2_filesys_test_2() {
	int32_t fd;
	int32_t length;
	int32_t nbytes = 1000;
	char buf[nbytes+1];

	printf("--------------------\n");

	fd = file_open((uint8_t*)"frame1.txt");
	length = file_read(fd, (uint8_t*)buf, nbytes);
	buf[length+1] = '\0';
	printf("File name: frame1.txt\n");
	printf("Bytes readed: %d\n", length);
	printf("Content:\n%s\n", buf);
	file_close(fd);

	return PASS;
}

/* Check point 2.3 (File system)
 * Coverage: read executables
 * Files: file_sys/c/h
 */
int cp2_filesys_test_3() {
	int32_t fd;
	int32_t length;
	int32_t nbytes = 100000;
	int32_t i;
	uint8_t buf[nbytes+1];

	/* We eliminate the '\0' character */

	printf("--------------------\n");

	fd = file_open((uint8_t*)"ls");
	length = file_read(fd, buf, nbytes);
	buf[length+1] = '\0';
	printf("File name: ls\n");
	printf("Bytes readed: %d\n", length);
	printf("Content:\n");
	// Print all characters
	for (i = 0; i < length; i++) {
		if (buf[i] != '\0')
			putc(buf[i]);
	}
	printf("\n");
	// Print first 100 characters
	// printf("--------------------\n");
	// printf("In addition, characters at begining:\n");
	// for (i = 0; i < 100; i++) {
	// 	if (buf[i] != '\0')
	// 		putc(buf[i]);
	// }
	// printf("\n");

	file_close(fd);

	return PASS;
}

/* Check point 2.3 (File system)
 * Coverage: read large files
 * Files: file_sys/c/h
 */
int cp2_filesys_test_4() {
	int32_t fd;
	int32_t length;
	int32_t nbytes = 100000;
	char buf[nbytes+1];

	printf("--------------------\n");

	fd = file_open((uint8_t*)"verylargetextwithverylongname.tx");		// use fish / verylargetextwithverylongname.tx
	length = file_read(fd, (uint8_t*)buf, nbytes);
	buf[length+1] = '\0';
	printf("File name: verylargetextwithverylongname.tx\n");
	printf("Bytes readed: %d\n", length);
	printf("Content:\n%s", buf);
	file_close(fd);

	return PASS;
}

/* Check point 2.3 (File system)
 * Coverage: open, close, write / handle error condition
 * Files: file_sys/c/h
 */
int cp2_filesys_test_5() {
	int32_t fd, fd1, fd2, fd3;
	int32_t nbytes = 40;
	char buf[nbytes+1];


	printf("--------------------\n");
	printf("I. Open, close, write, direct_read:\n");

	fd1 = file_open((uint8_t*)"frame1.txt");
	fd2 = file_open((uint8_t*)"grep");
	fd3 = file_open((uint8_t*)"verylargetextwithverylongname.tx");
	fd = direct_open((uint8_t*)".");

	printf("file_open shoud return file descriptors: fd1=%d, fd2=%d, fd3=%d\n", fd1, fd2, fd3);	
	printf("direct_open shoud return 0: fd=%d\n", fd);	
	printf("file_write shoud return -1: %d\n",file_write(fd1, (uint8_t*)buf, nbytes));
	printf("direct_write shoud return -1: %d\n",file_write(fd1, (uint8_t*)buf, nbytes));

	printf("direct_read should read file name:\n");
	direct_read(fd, (uint8_t*)buf, nbytes);
	printf("File name of fd: %s\n", buf);
	direct_read(fd1, (uint8_t*)buf, nbytes);
	printf("File name of fd1: %s\n", buf);
	direct_read(fd2, (uint8_t*)buf, nbytes);
	printf("File name of fd2: %s\n", buf);
	direct_read(fd3, (uint8_t*)buf, nbytes);
	printf("File name of fd3: %s\n", buf);

	printf("direct_close should return 0: %d\n", direct_close(fd));
	printf("file_close should return 0: %d\n", file_close(fd3));
	file_close(fd2);
	file_close(fd1);

	printf("--------------------\n");
	printf("II. handle error condition:\n");
	printf("Invalid file name 1 (return -1): %d\n", file_open((uint8_t*)"shel"));	
	printf("Invalid file name 2 (return -1): %d\n", file_open((uint8_t*)"shelll"));
	printf("Invalid directory name (return -1): %d\n", file_open((uint8_t*)".."));
	printf("Invalid file descriptor (return -1): %d\n", file_read(9, (uint8_t*)buf, nbytes));
	fd1 = file_open((uint8_t*)"rtc");
	printf("Try to read non-regular file (return -1): %d\n", file_read(fd1, (uint8_t*)buf, nbytes));

	return PASS;
}

/* Checkpoint 3 tests */
/* Checkpoint 4 tests */
/* Checkpoint 5 tests */


/* Test suite entry point */
void launch_tests(){
	/* Check point 1 */
	// TEST_OUTPUT("CP1_idt_test_1", CP1_idt_test_1());
	// TEST_OUTPUT("CP1_idt_test_2", CP1_idt_test_2());
    // TEST_OUTPUT("pic_test", pic_test());
	// TEST_OUTPUT("Paging Test", paging_test());
	// TEST_OUTPUT("Paging Test: Page Fault", paging_test_pf());

    /* Check point 2 */
    // term_read_write_test();
	// TEST_OUTPUT("File System test 1", cp2_filesys_test_1());		// list all file information
	// TEST_OUTPUT("File System test 2", cp2_filesys_test_2());		// read short file
	// TEST_OUTPUT("File System test 3", cp2_filesys_test_3());		// read executable
	// TEST_OUTPUT("File System test 4", cp2_filesys_test_4());		// read large file
	// TEST_OUTPUT("File System test 5", cp2_filesys_test_5());		// open, close, write / handle error condition
}
