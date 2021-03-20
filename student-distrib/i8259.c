/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"

/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7  */
uint8_t slave_mask;  /* IRQs 8-15 */

/* Initialize the 8259 PIC */
void i8259_init(void) {
    outb(MASK_ALL, MASTER_8259_DATA);
    outb(MASK_ALL, SLAVE_8259_DATA);

    outb(ICW1, MASTER_8259_PORT);	// ICW1: select 8259A-1 init
	outb(ICW1, SLAVE_8259_PORT);

	outb(ICW2_MASTER, MASTER_8259_DATA);
    outb(ICW2_SLAVE, SLAVE_8259_DATA);

	outb(ICW3_MASTER, MASTER_8259_DATA);
    outb(ICW3_SLAVE, SLAVE_8259_DATA);

    outb(ICW4, MASTER_8259_DATA);
    outb(ICW4, SLAVE_8259_DATA);
}

/* Enable (unmask) the specified IRQ */
void enable_irq(uint32_t irq_num) {
    unsigned int mask;
    mask = ~(1 << irq_num);
    master_mask &= mask;
    slave_mask &= (mask >> 8);

    // If IRQ is larger or equal to 8, slave
    if (irq_num >= MAS_SLA_DIV) {
        outb(slave_mask, SLAVE_8259_DATA);
    } else {
        // Else, master
        outb(master_mask, MASTER_8259_DATA);
    }
}

/* Disable (mask) the specified IRQ */
void disable_irq(uint32_t irq_num) {
    unsigned int mask;
    mask = 1 << irq_num;
    master_mask |= mask;
    slave_mask |= (mask >> 8);

    // If IRQ is larger or equal to 8, slave
    if (irq_num >= MAS_SLA_DIV) {
        outb(slave_mask, SLAVE_8259_DATA);
    } else {
        // Else, master
        outb(master_mask, MASTER_8259_DATA);
    }
}

/* Send end-of-interrupt signal for the specified IRQ */
void send_eoi(uint32_t irq_num) {
    // If IRQ is larger or equal to 8, slave
    if (irq_num >= MAS_SLA_DIV) {
        outb(EOI | (irq_num - MAS_SLA_DIV), SLAVE_8259_PORT);
        outb(EOI | SLAVE_IRQ, MASTER_8259_PORT);
    } else
        outb(EOI | irq_num, MASTER_8259_PORT);
}
