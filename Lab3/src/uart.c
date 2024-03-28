#include "aux.h"
#include "gpio.h"
#include "mystring.h"
#include "mymath.h"


#define uart_buf_len 256
static char rx_buf[uart_buf_len];
static char tx_buf[uart_buf_len];
static int rx_point = 0;
static int tx_point = 0;

/*=======================================================================+
 | Enable or Disable mini uart interrupt                                 |
 +======================================================================*/

/*************************************************************************
 * Disable Recieve interrupt without reset the rx_point
 ************************************************************************/
int disable_uart_receive_int(void) {
  *AUX_MU_IER &= 0x02;
  return 0;
}

/**************************************************************************
 * Enable Recieve interrupt
 *************************************************************************/
int enable_uart_receive_int(void) {
  // rx_point = 0;        // Initialize the pivot
  *AUX_MU_IER |= 0x01; // Enable Rx interrupt
  return 0;
}

/*************************************************************************
 * Disable Transmit interrupt without reset the rx_point
 ************************************************************************/
int disable_uart_transmit_int(void) {
  *AUX_MU_IER &= 0x01; // Disalbe bit 2.
  return 0;
}

/**************************************************************************
 * Enable Transmit interrupt
 *************************************************************************/
int enable_uart_transmit_int(void) {
  *AUX_MU_IER |= 0x02; // Enable Tx interrupt
  return 0;
}
/*=======================================================================*/

int reset_rx(void) {
  rx_point = 0;
  return 0;
}

/**************************************************************************
 * The interrupt handler of mini Uart Receive.
 *
 * Read all input into the rx_buf.
 *************************************************************************/
int uart_receive_handler() {
  // uart_puts("receive\n");
  if (rx_point >= uart_buf_len - 1)
    rx_point %= uart_buf_len;
  rx_buf[rx_point++] = (char)(*AUX_MU_IO);
  return 0;
}

/**************************************************************************
 * Interrupt handler of mini Uart Transmit
 *
 * Put all contents into uart and disable the TX interupt.
 *************************************************************************/
int uart_transmit_handler() {
  // uart_puts("transmit\n");
  if (tx_point < uart_buf_len && tx_buf[tx_point] != 0) {
    *AUX_MU_IO = tx_buf[tx_point++]; // Write to buffer
    enable_uart_transmit_int();      // Still have chars in the buffer.
  } else
    *AUX_MU_IER &= 0x01; // Transmition done disable interrupt.
  return 0;
}

/**************************************************************************
 * Interrupt version of sending string through UART
 *************************************************************************/
int uart_a_puts(const char *str, int len) {
  uart_puts("async write:\n");
  if (len <= 0)
    return 1;
  tx_point = 0;
  for (int i = 0; i < len; i++) {
    tx_buf[i] = str[i];
  }
  tx_buf[len] = 0;
  *AUX_MU_IER |= 0x02; // Enable Tx interrupt.
  return 0;
}

/**************************************************************************
 * Interrupt version of geting string
 * Should close the Rx interrupt after this function called.
 *************************************************************************/
int uart_a_gets(char *str, int len) {
  *AUX_MU_IER &= 0x02; // Disable Rx interrupt.
  if (len <= 0)
    return 1;
  for (int i = 0; i < rx_point && i < len; i++) {
    str[i] = rx_buf[i];
  }
  rx_point = 0;
  return 0;
}

void uart_setup() {

  volatile unsigned int r = *GPFSEL1;
  r &= ~(7 << 12); // Clear gpio14
  r |= (2 << 12);  // gpio14 to alt5

  r &= ~(7 << 15); // Clear gpio15
  r |= (2 << 15);  // gpio15 to alt5

  *GPFSEL1 = r; // Write register
  *GPPUD = 0;   // disable pullup/down
  volatile register unsigned int t;
  t = 150;
  // Waiting for seting
  // The GPIO Pull-up/down Clock Registers control the actuation of internal
  // pull-downs on the respective GPIO pins. These registers must be used in
  // conjunction with the GPPUD register to effect GPIO Pull-up/down changes.
  // The following sequence of events is required:
  // 1. Write to GPPUD to set the required control signal (i.e. Pull-up or
  // Pull-Down or neither to remove the current Pull-up/down)
  // 2. Wait 150 cycles – this provides the required set-up time for the control
  // signal
  // 3. Write to GPPUDCLK0/1 to clock the control signal into the GPIO pads you
  // wish to modify – NOTE only the pads which receive a clock will be modified,
  // all others will retain their previous state.
  // 4. Wait 150 cycles – this provides the required hold time for the control
  // signal
  // 5. Write to GPPUD to remove the control signal
  // 6. Write to GPPUDCLK0/1 to remove the clock
  while (t--) {
    asm volatile("nop");
  }
  *GPPUDCLK0 = (1 << 14) | (1 << 15); // Setup clock for gp14, 15
  t = 150;

  while (t--) {
    asm volatile("nop");
  }
  *GPPUDCLK0 = 0;

  *AUX_ENABLES |= 1;   // 1 -> AUX mini Uart
  *AUX_MU_CNTL = 0;   // Disable Tx/Rx
  *AUX_MU_LCR = 3;    // Set data to 8-bit mode
  *AUX_MU_MCR = 0;    // Ignore
  *AUX_MU_IER = 0x0;  // Enable both T/R interrupts(bit1/0).
  *AUX_MU_IIR = 0xc6; // No timeout + clear FIFO
  *AUX_MU_BAUD = 270; // 115200

  *AUX_MU_CNTL = 3; // Enable tr and re.
  return;
}

void uart1_init() {
    /* Initialize UART */
    *AUX_ENABLES |= 1;   // Enable mini UART
    *AUX_MU_CNTL = 0;    // Disable TX, RX during configuration
    *AUX_MU_IER = 0;     // Disable interrupt
    *AUX_MU_LCR = 3;     // Set the data size to 8 bit
    *AUX_MU_MCR = 0;     // Don't need auto flow control
    *AUX_MU_BAUD = 270;  // Set baud rate to 115200
    *AUX_MU_IIR = 0xc6;     // No FIFO

    /* Map UART to GPIO Pins */

    // 1. Change GPIO 14, 15 to alternate function
    register unsigned int r = *GPFSEL1;
    // r &= ~(7<<12);                   // clean gpio14
    // r |= 2<<12;                      // set alt5 for gpio14
    // r &= ~(7<<15);                   // clean gpio15
    // r |= 2<<15;                      // set alt5 for gpio 15
    r&=~((7<<12)|(7<<15)); // gpio14, gpio15
    r|=(2<<12)|(2<<15);    // alt5
    *GPFSEL1 = r;

    // 2. Disable GPIO pull up/down (Because these GPIO pins use alternate functions, not basic input-output)
    // Set control signal to disable
    *GPPUD = 0;
    // Wait 150 cycles
    r = 150;
    while (r--) {
        asm volatile("nop");
    }
    // Clock the control signal into the GPIO pads
    *GPPUDCLK0 = (1 << 14) | (1 << 15);
    // Wait 150 cycles
    r = 150;
    while (r--) {
        asm volatile("nop");
    }
    // Remove the clock
    *GPPUDCLK0 = 0;

    // 3. Enable TX, RX
    *AUX_MU_CNTL = 3;
}

char uart1_recv() {
    // Check data ready field
    do {
        asm volatile("nop");
    } while (!(*AUX_MU_LSR & 0x01));
    // Read
    char r = (char)(*AUX_MU_IO);
    // Convert carrige return to newline
    return r == '\r' ? '\n' : r;
}

void uart1_send(unsigned int c) {
    // Check transmitter idle field
    do {
        asm volatile("nop");
    } while (!(*AUX_MU_LSR & 0x20));
    // send
    *AUX_MU_IO = c;
}
void uart1_printf(char* fmt, ...) {
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    extern volatile unsigned char _end;  // defined in linker
    char* s = (char*)&_end;              // put temporary string after code
    //
    //vsprintf(s, fmt);//, args);
    //
    //unsigned int vsprintf(char *dst, char *fmt){//, __builtin_va_list args){
    
    //char *dst_orig = s;
    char *dst = s;
    while (*fmt) {
        
        if (*fmt == '%') {
            fmt++;
            // escape %
            if (*fmt == '%') {
                goto put;
            }
            // ascii to char
            if (*fmt == 'c') {
                int arg = __builtin_va_arg(args, int);
                char buf[11];
                // change to decimal
                char *p = itoa(arg, buf);
                char *pp = p;
                int len = 0;
                // count len
                while(*pp){
                    len++;
                    pp++;
                }
                int tmp = 0;
                for (int i=0;i<len;i++){
                    tmp = tmp + (*p-'0') * pow(10, len-i-1);
                    p++;
                }
                // change decimal to ascii
                char character = (char)(tmp);
                *dst++ = character;
            }
            // string
            if (*fmt == 's') {
                char *p = __builtin_va_arg(args, char *);
                while (*p) {
                    *dst++ = *p++;
                }
            }
            // number
            if (*fmt == 'd') {
                int arg = __builtin_va_arg(args, int);
                char buf[11];
                char *p = itoa(arg, buf);
                while (*p) {
                    *dst++ = *p++;
                }
            }
            // float
            if (*fmt == 'f') {
                float arg = (float) __builtin_va_arg(args, double);
                char buf[19];  // sign + 10 int + dot + 6 float
                char *p = ftoa(arg, buf);
                while (*p) {
                    *dst++ = *p++;
                }
            }
            // hex
            if (*fmt == 'x') {
                // get %x's value
                int arg = __builtin_va_arg(args, int);
                char buf[8 + 1];
                char *p = itox(arg, buf);
                while (*p) {
                    *dst++ = *p++;
                }
            }
        } else {
        put:
            *dst++ = *fmt;
            
        }
        fmt++;
       
    }
    *dst = '\0';
    //
    while (*s) {
        if (*s == '\n') uart1_send('\r');
        uart1_send(*s++);
    }
}

void uart1_flush() {
    while (*AUX_MU_LSR & 0x01) {
        *AUX_MU_IO;
    }
}


void uart1_puts(char *s) {
    while(*s) {
        /* convert newline to carriage return + newline */
        if(*s=='\n')
            uart1_send('\r');
        uart1_send(*s++);
    }
}

