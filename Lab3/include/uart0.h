void uart0_init();
void uart0_send(unsigned int c);
char uart0_recv();
void uart0_printf(char* fmt, ...);
void uart0_flush();
void uart0_puts(char *s);