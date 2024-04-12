#include "uart.h"
unsigned long long cntfrq_el0;
unsigned long long cntpct_el0;
long long int get_timestamp() {
    asm volatile ("mrs %0, cntfrq_el0" : "=r" (cntfrq_el0)); // get current counter frequency
    asm volatile ("mrs %0, cntpct_el0" : "=r" (cntpct_el0)); // read current counter
    // uart_printf("cntpct_el0: %d\n", cntpct_el0);
    // uart_printf("cntfrq_el0: %d\n", cntfrq_el0);
    uart_printf("timestamp: %d\n", (long long int)cntpct_el0 / cntfrq_el0);
    return (long long int) cntpct_el0 / cntfrq_el0;
}