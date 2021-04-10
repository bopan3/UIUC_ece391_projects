/*
 * Header file for handler functinos
 */

#ifndef HANDLERS_H
#define HANDLERS_H

/* Save registers and pass control to a interrupt handler specified by irq_vect */
extern void irq_handler(int irq_vect);

#endif
