void uart_init();
void uart_send(unsigned int c);
char uart_recv();
void uart_printf(char* fmt, ...);
void uart_flush();
void uart_puts(char *s);

// Enable uart recieve handler
int enable_uart_receive_int(void);
int disable_uart_receive_int(void);
int enable_uart_transmit_int(void);
int disable_uart_transmit_int(void);
int uart_transmit_handler();
int uart_receive_handler();
int uart_a_puts(const char *str, int len);
int uart_a_gets(char *str, int len);
void uart_show_a_recv();