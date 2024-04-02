#include "peripherals/mmio.h"

#ifndef IRQ_H
#define IRQ_H

#define IRQ_BASE                (MMIO_BASE + 0xb000)
#define GPU_INT_ROUT            ((volatile unsigned int *)0x4000000C)
#define IRQ_PEND_1             ((unsigned int *)(IRQ_BASE + 0x204))
#define IRQ_BASIC_PENDING       ((unsigned int*)(IRQ_BASE + 0x200))
#define IRQ_ENABLE_1            ((unsigned int*)(IRQ_BASE + 0x210))
/// Interrupt enable/disable functions.
int enable_int(void);
int disable_int(void);
int mini_uart_interrupt_enable(void);
#endif