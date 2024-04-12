#include "command.h"
#include "heap.h"
#include "initrd.h"
#include "loader.h"
#include "mailbox.h"
#include "str.h"
#include "terminal.h"
#include "timer.h"
#include "uart.h"
#include "util.h"
// static char buf[256];

struct command commands[] = {{
                                 .name = "help",
                                 .help = "Show help message!\n",
                                 .func = help,
                             },
                             {
                                 .name = "lshw",
                                 .help = "Show some HW informations\n",
                                 .func = lshw,
                             },
                             {
                                 .name = "hello",
                                 .help = "Print \'hello world\'\n",
                                 .func = hello,
                             },
                             {
                                 .name = "reboot",
                                 .help = "Reboot the device.\n",
                                 .func = reboot,
                             },
                             {
                                 .name = "ls",
                                 .help = "List file name in FS.\n",
                                 .func = ls,
                             },
                             {
                                 .name = "cat",
                                 .help = "Show the content of target file.\n",
                                 .func = cat,
                             },
                             {
                                 .name = "run",
                                 .help = "Run the specific user program.\n",
                                 .func = run_loader,
                             },
                             {
                                 .name = "set timer",
                                 .help = "set a timer and show the message.\n",
                                 .func = time_out,
                             },
                             {
                                 .name = "read",
                                 .help = "Read by interrupt handler.\n",
                                 .func = async_read,
                             },
                             {
                                 .name = "timestamp",
                                 .help = "Get timestamp\n",
                                 .func = get_timestamp,
                             },

                             // ALWAYS The last item of the array!!!
                             {
                                 .name = "NULL", // The end of the array
                             }};

int async_read() {
  uart_puts("Async Read start...\n");
  enable_uart_receive_int();
  set_timer_read();
  delay(16000000);
  disable_uart_receive_int();
  uart_show_a_recv();
  return 0;
}
int time_out() {
  char *buf = (char *)malloc(sizeof(char) * 256);
  char integer[10];
  int second = 0;
  memset(buf, 0, 256);
  memset(integer, 0, 10);
  uart_puts("Message:\n");
  uart_gets(buf);
  uart_puts("Seconds:\n");
  uart_gets(integer);
  // char array -> int
  for (int i = 0; i < 10; i++) {
    if (integer[i] >= '0' && integer[i] <= '9') {
      second += integer[i] - '0';
      second *= 10;
    }
  }
  second /= 10;
  set_timeout(buf, second);
  uart_puts("done\n");

  return 1;
}

int run_loader() {
  char buf[256];
  memset(buf, 0, 256);
  void *start = 0;
  uart_puts("Name:\n");
  uart_gets(buf);
  unsigned long fileSize;
  start = cpio_get_file((void *)initrd_getLo(), buf, &fileSize);
  if (start != 0) {
    uart_puth(start);
    uart_printf("%d",fileSize);
    // uart_puth(*(int*)start);
    run_program(start);
    return 0;
  }
  return 1;
}

int help() {
  int i = 0;
  while (1) {
    if (!strcmp(commands[i].name, "NULL")) {
      break;
    }
    uart_puts(commands[i].name);
    uart_puts(": ");
    uart_puts(commands[i].help);
    i++;
  }
  return 0;
}

int cat() {
  char buf[256];
  memset(buf, 0, 256);
  uart_puts("Name:\n");
  uart_gets(buf);
  unsigned long fileSize;
  char *result = cpio_get_file((void *)initrd_getLo(), buf, &fileSize);
  if (result != '\0') {
      for (int i = 0;i < fileSize;i++) {
          uart_printf("%c", result[i]);
      }
      uart_printf("\n");
  } else {
      uart_printf("'%s' file  not exist!\n", buf);
  }
  // initrd_cat("boot.S");
  return 0;
}

int ls() {
  cpio_ls((void *)initrd_getLo());
  return 0;
}

int hello() {
  uart_puts("Hello World!\n");
  return 0;
}

int invalid_command(const char *s) {
  uart_putc('`');
  uart_puts(s);
  uart_putc('`');
  uart_puts(" is invalid command! Please use `help` to list commands\n");
  return 0;
}

int lshw(void) {
  uart_puts("Board version\t: ");
  mbox[0] = 7 * 4;
  mbox[1] = MAILBOX_REQ;
  mbox[2] = TAG_BOARD_VER;
  mbox[3] = 4;
  mbox[4] = 0;
  mbox[5] = 0;
  mbox[6] = TAG_LAST;

  if (mailbox_config(CHANNEL_PT)) {
    uart_puth(mbox[5]);
  }
  uart_puts("\nDevice base Mem Addr\t: ");
  mbox[0] = 8 * 4;
  mbox[1] = MAILBOX_REQ;
  mbox[2] = TAG_ARM_MEM;
  mbox[3] = 8;
  mbox[4] = 0;
  mbox[5] = 0;
  mbox[6] = 0;
  mbox[7] = TAG_LAST;
  if (mailbox_config(CHANNEL_PT)) {
    uart_puth(mbox[5]);
    uart_puts("\nDevice Mem size\t: ");
    uart_puth(mbox[6]);
  }
  uart_putc('\n');
  return 0;
}

int reboot() {
  *PM_RSTC = PM_PASSWORD | 0x20; // Reset
  *PM_WDOG = PM_PASSWORD | 180;
  return 0;
}
