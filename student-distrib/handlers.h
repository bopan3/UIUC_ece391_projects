/*
 * Header file for handler functinos
 */

#ifndef HANDLERS_H
#define HANDLERS_H

/* Save registers and pass control to a interrupt handler specified by irq_vect */
extern void irq_handler(int irq_vect);

/* Save registers and pass control to a system call handler specified by exp_vect */
extern void sys_handler(int exp_vect);

#endif
