#include "peripherals/uart0.h"
#include "mbox.h"
#include "mystring.h"
#include "peripherals/gpio.h"
#include "peripherals/mbox.h"

void uart0_init() {
    *UART0_CR = 0;  // turn off UART0

    /* Configure UART0 Clock Frequency */
    unsigned int __attribute__((aligned(16))) mbox[9];
    mbox[0] = 9 * 4;
    mbox[1] = MBOX_CODE_BUF_REQ;
    // tags begin
    mbox[2] = MBOX_TAG_SET_CLOCK_RATE;
    mbox[3] = 12;
    mbox[4] = MBOX_CODE_TAG_REQ;
    mbox[5] = 2;        // UART clock
    mbox[6] = 4000000;  // 4MHz
    mbox[7] = 0;        // clear turbo
    mbox[8] = 0x0;      // end tag
    // tags end
    mbox_call(mbox, 8);

    /* Map UART to GPIO Pins */
    // 1. Change GPIO 14, 15 to alternate function
    register unsigned int r = *GPFSEL1;
    r &= ~((7 << 12) | (7 << 15));  // Reset GPIO 14, 15
    r |= (4 << 12) | (4 << 15);     // Set ALT0
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

    /* Configure UART0 */
    *UART0_IBRD = 0x2;        // Set 115200 Baud
    *UART0_FBRD = 0xB;        // Set 115200 Baud
    *UART0_LCRH = 0b11 << 5;  // Set word length to 8-bits
    *UART0_ICR = 0x7FF;       // Clear Interrupts

    /* Enable UART */
    *UART0_CR = 0x301;
}

char uart0_recv() {
    // Check data ready field
    do {
        asm volatile("nop");
    } while (*UART0_FR & 0x10);
    // Read
    char r = (char)(*UART0_DR);
    // Convert carrige return to newline
    return r == '\r' ? '\n' : r;
}

void uart0_send(unsigned int c) {
    // Check transmitter idle field
    do {
        asm volatile("nop");
    } while (*UART0_FR & 0x20);
    // Write
    *UART0_DR = c;
}

void uart0_printf(char* fmt, ...) {
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
        if (*s == '\n') uart0_send('\r');
        uart0_send(*s++);
    }
}

void uart0_flush() {
    while (!(*UART0_FR & 0x10)) {
        (void)*UART0_DR;  // unused variable
    }
}
void uart0_puts(char *s) {
    while(*s) {
        /* convert newline to carriage return + newline */
        if(*s=='\n')
            uart0_send('\r');
        uart0_send(*s++);
    }
}

char uart0_recv_raw() {
    do {
        asm volatile("nop");
    } while (*UART0_FR & 0x10);
    return (char)(*UART0_DR);
}
