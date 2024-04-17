#include "dtb.h"
#include "exception.h"
#include "heap.h"
#include "initrd.h"
#include "interrupt.h"
#include "mem.h"
#include "terminal.h"
#include "uart.h"
#include <stdint.h>

extern void set_exception_vector_table(void);

int main(void *dtb_location) {
  uart_setup();
  heap_init();
  pmalloc_init();
  set_exception_vector_table();
  enable_int();
  // preserve(0, 0x5000000);
  // preserve(0x8200000, 0x16000);
  
  // For Spin Table of Multicore boot
	preserve(0x0, 0x2000);
  // For Heap and Code Section
	preserve(0x80000, 0x20000);
	// For Initramfs.cpio?????????
	preserve(0x20000000, 0x1000);
	// For DTB????????
	preserve(dtb_location, 0x18000);

  smalloc_init();
  uart_puts("==================\n");

  // void *a = pmalloc(4);
  // void *b = pmalloc(4);
  // void *c = pmalloc(4);
  // void *d = pmalloc(0);
  // void *e = pmalloc(0);
  // void *f = pmalloc(0);
  // pfree(d);
  // pfree(e);
  // pfree(f);
  // pfree(c);
  // pfree(b);
  // pfree(a);
  // void *sa = smalloc(4);
  // void *sb = smalloc(4);
  // void *se = smalloc(4);
  // void *sc = smalloc(16);
  // void *sd = smalloc(15);
  // uart_puts("smem test\n");
  // uart_puth(sa);
  // uart_puts("\n");
  // uart_puth(sb);
  // uart_puts("\n");
  // uart_puth(se);
  // uart_puts("\n");
  // uart_puth(sc);
  // uart_puts("\n");
  // uart_puth(sd);
  // uart_puts("\n");
  // core_timer_enable();
  fdt_find_do(dtb_location, "linux,initrd-start", initrd_fdt_callback);
  terminal_run();

  return 0;
}
