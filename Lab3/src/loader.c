#include "loader.h"
#include "peripherals/irq.h"
#include "uart.h"
#include "timer.h"
#include "heap.h"

/**************************************************************************
 * This function will run the program at the specific location.
 * And this function sould handle following tasks.
 * 1. Save kernel status.
 * 2. EL1 -> EL0.
 * 3. Start user program.
 * ***********************************************************************/
int run_program(void *loc, unsigned long fileSize) {
  // arm_core_timer_enable();
  // mini_uart_interrupt_enable();
  char * ptr = (char *)malloc(fileSize);
  char * sptr = (char *)malloc(4096);
  char *locc = (char *)loc;
  for (int i=0; i<fileSize ;i++){
    ptr[i] = locc[i];
  }

  asm volatile("mov x1,	0x3c0;\r\n" 
               "msr spsr_el1, 	x1;\r\n"
               "mov x1,	%[sp];\r\n" // Set user stack to 0x60000
               "msr sp_el0,	x1;\r\n"
               "mov x1,	%[loc];\r\n"
               "msr elr_el1,	x1;\r\n" // Set the target address
                // "mov x0,	%[loc];\r\n"     // For recalculat offset
               "eret"
               :
               : [sp] "r"(sptr),[loc] "r"(ptr)
               : // input location
  );

  // Note: at this stage, this function will not return.
  return 0;
}
