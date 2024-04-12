#include "dtb.h"
#include "exception.h"
#include "heap.h"
#include "initrd.h"
#include "interrupt.h"
#include "terminal.h"
#include "uart.h"
#include <stdint.h>

extern void set_exception_vector_table(void);

int main(void *dtb_location) {
  uart_setup();
  heap_init();
  set_exception_vector_table();
  core_timer_enable();
  fdt_find_do(dtb_location, "linux,initrd-start", initrd_fdt_callback);
  uart_puth(*(int*)dtb_location);
  enable_int();
  // uart_a_puts("testuart_async\n", 16);
  // enable_uart_receive_int();
  // // key character
  // for (int i=0;i<10000000;i++){
  //   asm volatile("nop");
  // }
  // // show character typed during for loop
  // disable_uart_receive_int();
  // uart_show_a_recv();
  asm volatile("svc 0;");
  terminal_run();
  return 0;
}
