#ifndef __SHARED_VAR_H__
#define __SHARED_VAR_H__

#include "queue.h"
#define UART0_BUF_MAX_SIZE 1024
extern struct queue read_buf, write_buf;

extern unsigned long long arm_core_timer_jiffies;
extern unsigned long long arm_local_timer_jiffies;

extern unsigned long long cntfrq_el0;
extern unsigned long long cntpct_el0;

#endif

void shared_variables_init();