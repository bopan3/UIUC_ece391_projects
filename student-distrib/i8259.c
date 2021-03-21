/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"

/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7  */
uint8_t slave_mask;  /* IRQs 8-15 */

/*
 *   i8259_init
 *   DESCRIPTION: Initialize the 8259 PIC
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Initialize master and slave PICs
 */
void i8259_init(void) {
    outb(ICW1, MASTER_8259_PORT);	// ICW1: select 8259A-1 init
	outb(ICW1, SLAVE_8259_PORT);

	outb(ICW2_MASTER, MASTER_8259_DATA);
    outb(ICW2_SLAVE, SLAVE_8259_DATA);

	outb(ICW3_MASTER, MASTER_8259_DATA);
    outb(ICW3_SLAVE, SLAVE_8259_DATA);

    outb(ICW4, MASTER_8259_DATA);
    outb(ICW4, SLAVE_8259_DATA);

    outb(MASK_ALL, MASTER_8259_DATA);
    outb(MASK_ALL, SLAVE_8259_DATA);
}

/*
 *   enable_irq
 *   DESCRIPTION: Enable (unmask) the specified IRQ
 *   INPUTS: irq_num -- the bit number of interruption
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: switch the given bit of IRQ to 0
 */
void enable_irq(uint32_t irq_num) {
    unsigned int mask;
    mask = ~(1 << irq_num);         // Here 1 is just a positive bit
    master_mask &= mask;
    slave_mask &= (mask >> MAS_SLA_DIV);

    // If IRQ is larger or equal to 8, slave
    if (irq_num >= MAS_SLA_DIV)
        outb(slave_mask, SLAVE_8259_DATA);
    else {
        // Else, master
        outb(master_mask, MASTER_8259_DATA);
    }
}

/*
 *   disable_irq
 *   DESCRIPTION: Disable (mask) the specified IRQ
 *   INPUTS: irq_num -- the bit number of interruption
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: switch the given bit of IRQ to 1
 */
void disable_irq(uint32_t irq_num) {
    unsigned int mask;
    mask = 1 << irq_num;            // Here 1 is just a positive bit
    master_mask |= mask;
    slave_mask |= (mask >> MAS_SLA_DIV);

    // If IRQ is larger or equal to 8, slave
    if (irq_num >= MAS_SLA_DIV)
        outb(slave_mask, SLAVE_8259_DATA);
    else {
        // Else, master
        outb(master_mask, MASTER_8259_DATA);
    }
}

/*
 *   send_eoi
 *   DESCRIPTION: Send end-of-interrupt signal for the specified IRQ
 *   INPUTS: irq_num -- the bit number of interruption
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Send end-of-interrupt signal for the specified IRQ
 */
void send_eoi(uint32_t irq_num) {
    // If IRQ is larger or equal to 8, slave
    if (irq_num >= MAS_SLA_DIV) {
        outb(EOI | (irq_num - MAS_SLA_DIV), SLAVE_8259_PORT);
        outb(EOI | SLAVE_IRQ, MASTER_8259_PORT);
    } else
        outb(EOI | irq_num, MASTER_8259_PORT);
}
