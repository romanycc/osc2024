#include "peripherals/mmio.h"

#ifndef UART_H
#define UART_H

#define UART_BASE      (MMIO_BASE + 0x205000)

#define AUX_IRQ         ((unsigned int*)(UART0_BASE)+ 0x00)
#define AUX_ENABLES     ((unsigned int*)(UART0_BASE)+ 0x04)
#define AUX_MU_IO_REG   ((unsigned int*)(UART0_BASE)+ 0x40)
#define AUX_MU_IER_REG  ((unsigned int*)(UART0_BASE)+ 0x44)
#define AUX_MU_IIR_REG  ((unsigned int*)(UART0_BASE)+ 0x48)
#define AUX_MU_LCR_REG  ((unsigned int*)(UART0_BASE)+ 0x4c)
#define AUX_MU_MCR_REG  ((unsigned int*)(UART0_BASE)+ 0x50)
#define AUX_MU_LSR_REG  ((unsigned int*)(UART0_BASE)+ 0x54)
#define AUX_MU_MSR_REG  ((unsigned int*)(UART0_BASE)+ 0x58)
#define AUX_MU_SCRATCH  ((unsigned int*)(UART0_BASE)+ 0x5c)
#define AUX_MU_CNTL_REG ((unsigned int*)(UART0_BASE)+ 0x60)
#define AUX_MU_STAT_REG ((unsigned int*)(UART0_BASE)+ 0x64)
#define AUX_MU_BAUD_REG ((unsigned int*)(UART0_BASE)+ 0x68)

#endif