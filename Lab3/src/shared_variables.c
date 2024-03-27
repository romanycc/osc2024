#include "shared_variables.h"

struct queue read_buf, write_buf;

unsigned long long arm_core_timer_jiffies;
unsigned long long arm_local_timer_jiffies;

unsigned long long cntfrq_el0;
unsigned long long cntpct_el0;

void shared_variables_init() {
    arm_core_timer_jiffies = 0;
    arm_local_timer_jiffies = 0;

    queue_init(&read_buf, UART0_BUF_MAX_SIZE);
    queue_init(&write_buf, UART0_BUF_MAX_SIZE);
}