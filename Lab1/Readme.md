# Lab 1 - Hello World

## Code Structure
* include
    * aux.h
    * gpio.h
    * mmio.h
    * mymath.h
    * mtstring.h
    * shell.h
    * uart.h
    * util.h
    * /peripherals/mbox.h
* src
    * linker.ld
    * start.s
    * main.c
    * mymath.c
    * mystring.c
    * shell.c
    * uart.c
    * util.c
    * mbox.c
* Makefile

## Code Explanation
### include
#### mymath.h
* declare pow function 
#### mystring.h
* declare itox(int to hex), itoa(int to char), strcmp
#### shell.h
* declare shell_init, shell_input, shell_controller
#### uart.h
* declare uart_init, uart_send, uart_recv, uart_printf, uart_flush
#### util.h
* get_timestamp, reset, cancel_reset
#### mmio.h
* 在 Raspberry Pi 3 中，MMU 將物理地址 0x3F000000 映射到匯流排地址 0x7E000000。這意味著當程式碼（使用者）存取物理地址 0x3F000000 時，實際上是在存取匯流排地址 0x7E000000。
```=C
#ifndef MMIO_H
#define MMIO_H
#define MMIO_BASE       0x3F000000
#endif
```
* 舉例來說：在這段程式碼中，gpio_base 指向物理地址 0x3F200000。但是，由於 MMU 的存在，當程式碼存取 gpio_base 時，實際上是訪問了匯流排地址 0x7E200000。
```=c
#include <stdio.h>

int main() {
  // 訪問 GPIO 的寄存器
  volatile unsigned int *gpio_base = (volatile unsigned int *)0x3F200000;

  // 讀取 GPIO 引腳的狀態
  unsigned int gpio_status = *gpio_base;

  // 輸出 GPIO 引腳的狀態
  printf("GPIO 狀態：0x%x\n", gpio_status);

  return 0;
}
```
#### gpio.h
* [BCM2837 ARM Peripherals Page90](https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf)
![截圖 2024-02-27 上午11.50.39](https://hackmd.io/_uploads/SJyQnC92p.png)
```=C
#include "mmio.h"

#define GPIO_BASE       (MMIO_BASE + 0x200000) 

#define GPFSEL0         ((volatile unsigned int*)(GPIO_BASE + 0x00))
#define GPFSEL1         ((volatile unsigned int*)(GPIO_BASE + 0x04))
#define GPFSEL2         ((volatile unsigned int*)(GPIO_BASE + 0x08))
#define GPFSEL3         ((volatile unsigned int*)(GPIO_BASE + 0x0C))
#define GPFSEL4         ((volatile unsigned int*)(GPIO_BASE + 0x10))
#define GPFSEL5         ((volatile unsigned int*)(GPIO_BASE + 0x14))
// 0x18 Reserved
```
#### aux.h
![截圖 2024-02-27 下午12.05.45](https://hackmd.io/_uploads/HJnoykjnT.png)

```=c
#include "mmio.h"

#define AUX_BASE        (MMIO_BASE + 0x215000)

#define AUX_IRQ         ((volatile unsigned int*)(AUX_BASE + 0x00))
#define AUX_ENABLES     ((volatile unsigned int*)(AUX_BASE + 0x04))
#define AUX_MU_IO       ((volatile unsigned int*)(AUX_BASE + 0x40))
#define AUX_MU_IER      ((volatile unsigned int*)(AUX_BASE + 0x44))
#define AUX_MU_IIR      ((volatile unsigned int*)(AUX_BASE + 0x48))
#define AUX_MU_LCR      ((volatile unsigned int*)(AUX_BASE + 0x4C))
```
### src
#### linker.ld
* [Writing linker scripts and section placement](https://www.youtube.com/watch?v=B7oKdUvRhQQ)
![截圖 2024-02-27 下午2.57.49](https://hackmd.io/_uploads/BJ-W_bo2a.png)
![截圖 2024-02-27 下午2.55.08](https://hackmd.io/_uploads/r1OIPWo3p.png)
![截圖 2024-02-27 下午2.55.49](https://hackmd.io/_uploads/HkltwZina.png)
```
SECTIONS
{
  . = 0x80000;
  .text :
  {
    KEEP(*(.text.boot))
    *(.text)
  }
  .data :
  {
    *(.data)
  }
  .bss ALIGN(16) (NOLOAD) :
  {
    __bss_start = .;

    *(.bss)

    __bss_end = .;
  }
  _end = .;
}

__bss_size = (__bss_end - __bss_start) >> 3;
```
#### start.s
![截圖 2024-02-27 下午3.18.33](https://hackmd.io/_uploads/HkOCnZjna.png)
* Multiprocessor Affinity Register，此暫存器保存了processor ID
* mrs：Move to Register from State register
* cbz：Compare and Branch on Zero
* wfe：Wait for event
* ldr：load register
* str：store register
* xzr：通用寄存器中的一個特殊寄存器，它的值始終為零
* ```str     xzr, [x1], #8```將零值（即 xzr 寄存器的值）存儲到位於 x1 地址處的內存中，然後將 x1 的值增加 8 個字節，以便下一次存儲操作。

```=asm
.section ".text.boot"

.global _start

_start:
    // get cpu id
    mrs     x1, MPIDR_EL1
    and     x1, x1, #3
    cbz     x1, 2f
    // 2f：forward
    // if cpu_id > 0, stop
1:
    wfe
    b       1b
    // 1b：backward
    // if cpu_id == 0
2:
    // set stack pointer
    ldr     x1, =_start
    mov     sp, x1

    // clear bss
    ldr     x1, =__bss_start
    ldr     x2, =__bss_size
    // if size==0, done
3:  cbz     x2, 4f
    str     xzr, [x1], #8
    sub     x2, x2, #1
    cbnz    x2, 3b

    // jump to main function in C
4:  bl      main
    // halt this core if return
    b       1b
```
#### main.c
```=C
#include "shell.h"

#define CMD_LEN 128

enum shell_status{
    Read,
    Parse
};

int main(){
    shell_init();
    enum shell_status status = Read;
    while(1){
        char cmd[CMD_LEN];
        switch(status){
            case Read:
                shell_input(cmd);
                status = Parse;
                break;
            case Parse:
                shell_controller(cmd);
                status = Read;
                break;
        }
    }
}
```
#### mymath.c
```=C
double pow(double x, double y){
    if (y==0){
        return 1;
    }
    return  x * pow(x, y-1);
}
```
#### mystring.c
* Finish itoa, ftoa, strcmp function
#### uart.c
* [1.1: Introducing RPi OS, or bare-metal "Hello, World!"](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/docs/lesson01/rpi-os.md)
##### uart_init()
```=C
// 1. Change GPIO 14, 15 to alternate function
    register unsigned int r = *GPFSEL1;
    r &= ~(7<<12);                   // clean gpio14
    r |= 2<<12;                      // set alt5 for gpio14
    r &= ~(7<<15);                   // clean gpio15
    r |= 2<<15;                      // set alt5 for gpio 15
    *GPFSEL1 = r;
```
* 在嵌入式系統中，例如在 Raspberry Pi 上，許多針腳（GPIO pins）具有多種功能。這些針腳可以連接到不同的裝置或執行不同的任務。在使用特定針腳之前，我們需要確定該針腳的功能或設置，這就是所謂的替代功能（alternative function）。替代功能其實就是對每個針腳指定一個數字，通常從0到5的範圍內，這個數字對應著一個特定的功能配置。換句話說，每個針腳可以根據設置成不同的替代功能來執行不同的操作，比如作為普通的數位輸入輸出、I2C、SPI、UART等通信協定的介面，或者是其他的特定功能。
* Here you can see that pins 14 and 15 have the TXD1 and RXD1 alternative functions available. This means that if we select alternative function number 5 for pins 14 and 15, they will be used as a Mini UART Transmit Data pin and Mini UART Receive Data pin, respectively. The GPFSEL1 register is used to control alternative functions for pins 10-19. 
![截圖 2024-02-27 下午3.38.51](https://hackmd.io/_uploads/HJYqZMina.png)
![截圖 2024-02-27 下午3.49.41](https://hackmd.io/_uploads/rJZmVfshp.png)
![截圖 2024-02-27 下午3.49.22](https://hackmd.io/_uploads/Hk-GNzonT.png)
* 在使用 GPIO 針腳之前，我們總是需要初始化它們的狀態。通常有三種可用的狀態：拉上、拉下和無（去除當前的拉上或拉下電阻狀態）。在切換針腳狀態時，需要通過物理上切換電路中的開關，這個過程涉及 GPPUD 和 GPPUDCLK 寄存器，在 BCM2837 ARM Peripherals 手冊的第 101 頁有詳細描述。
```
The GPIO Pull-up/down Clock Registers control the actuation of internal pull-downs on
the respective GPIO pins. These registers must be used in conjunction with the GPPUD
register to effect GPIO Pull-up/down changes. The following sequence of events is
required:
1. Write to GPPUD to set the required control signal (i.e. Pull-up or Pull-Down or neither
to remove the current Pull-up/down)
2. Wait 150 cycles – this provides the required set-up time for the control signal
3. Write to GPPUDCLK0/1 to clock the control signal into the GPIO pads you wish to
modify – NOTE only the pads which receive a clock will be modified, all others will
retain their previous state.
4. Wait 150 cycles – this provides the required hold time for the control signal
5. Write to GPPUD to remove the control signal
6. Write to GPPUDCLK0/1 to remove the clock
```
```=C
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
```
##### uart_recv()
* We are using the AUX_MU_LSR_REG register to do this. Bit zero, if set to 1, indicates that the data is ready; this means that we can read from the UART. 
* Use AUX_MU_IO_REG to either store the value of the transmitted character or read the value of the received character.
```=C
char uart_recv() {
    // Check data ready field
    do {
        asm volatile("nop");
    } while (!(*AUX_MU_LSR & 0x01));
    // Read
    char r = (char)(*AUX_MU_IO);
    // Convert carrige return to newline
    return r == '\r' ? '\n' : r;
}
```
##### uart_send(unsigned int c)
* Bit five, if set to 1, tells us that the transmitter is empty, meaning that we can write to the UART. Next, we use AUX_MU_IO_REG to either store the value of the transmitted character or read the value of the received character.
```=C
void uart_send(unsigned int c) {
    // Check transmitter idle field
    do {
        asm volatile("nop");
    } while (!(*AUX_MU_LSR & 0x20));
    // send
    *AUX_MU_IO = c;
}
```
##### uart_printf(char* fmt, ...)
* 將格式化的字串輸出到 UART 串口
```=C
void uart_printf(char* fmt, ...) {
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    extern volatile unsigned char _end;  // defined in linker
    char* s = (char*)&_end;              // put temporary string after code
    //
    //vsprintf(s, fmt);//, args);
    //
    //unsigned int vsprintf(char *dst, char *fmt){//, __builtin_va_list args){
    
    char *dst_orig = s;
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
        if (*s == '\n') uart_send('\r');
        uart_send(*s++);
    }
}
```
#####  uart_flush()
* ```{ *AUX_MU_IO; }```：迴圈的主體內容是 *AUX_MU_IO;。這看起來像一個空的語句，但實際上它用來讀取 AUX_MU_IO 寄存器的值。通常，讀取這個寄存器的值是為了清空 UART 的接收緩衝區。
```=C
void uart_flush() {
    while (*AUX_MU_LSR & 0x01) {
        *AUX_MU_IO;
    }
}
```
#### shell.c
##### shell_init()
```=C
void shell_init(){
    uart_init();
    uart_flush();
    uart_printf("\n\nWelcome to RPI3\n");
}
```
##### shell_input(char* cmd)
* Decode input
* [ANSI Control Functions Summary](https://vt100.net/docs/vt510-rm/chapter4.html)
##### shell_controller(char* cmd)
* 實現各種功能：help, hello, reboot
#### util.c
```=C
#define PM_PASSWORD     0x5a000000
#define PM_RSTC         ((volatile unsigned int*)0x3F10001c)
#define PM_WDOG         ((volatile unsigned int*)0x3F100024)

float get_timestamp() {
    register unsigned long long f, c;
    asm volatile ("mrs %0, cntfrq_el0" : "=r"(f)); // get current counter frequency
    asm volatile ("mrs %0, cntpct_el0" : "=r"(c)); // read current counter
    return (float) c / f;
}

void reset(){ // reboot after watchdog timer expire
    *PM_RSTC = PM_PASSWORD | 0x20; // full reset
    *PM_WDOG = PM_PASSWORD | 100;   // number of watchdog tick
}

void cancel_reset() {
    *PM_RSTC = PM_PASSWORD | 0; // full reset
    *PM_WDOG = PM_PASSWORD | 0; // number of watchdog tick
}
```
#### Makefile
```=makefile
TOOLCHAIN_PREFIX = aarch64-linux-gnu-
CC = $(TOOLCHAIN_PREFIX)gcc
LD = $(TOOLCHAIN_PREFIX)ld
OBJCPY = $(TOOLCHAIN_PREFIX)objcopy

SRC_DIR = src
OUT_DIR = out

LINKER_FILE = $(SRC_DIR)/linker.ld
ENTRY = $(SRC_DIR)/start.s
ENTRY_OBJS = $(OUT_DIR)/start.o
// 這個語句使用了 wildcard 函數，用於匹配指定目錄下的所有 .c 文件，並傳回一個由檔案名稱組成的清單。 $(SRC_DIR)/*.c 表示在指定目錄 $(SRC_DIR) 中尋找所有的 .c 檔案。
// 對於每個來源檔案 src/*.c，它都會被替換為對應的目標檔案 out/*.o
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OUT_DIR)/%.o)

//  定義了編譯器選項。-Wall 表示啟用所有警告，-I include 指定了包含文件的路徑，-c 表示僅生成目標文件，不執行鏈接。
CFLAGS = -Wall -I include -c

.PHONY: all clean asm run debug directories

all: directories kernel8.img

$(ENTRY_OBJS): $(ENTRY)
	$(CC) $(CFLAGS) $< -o $@

$(OUT_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $< -o $@

kernel8.img: $(OBJS) $(ENTRY_OBJS)
	$(LD) $(ENTRY_OBJS) $(OBJS) -T $(LINKER_FILE) -o kernel8.elf
	$(OBJCPY) -O binary kernel8.elf kernel8.img

asm:
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -d in_asm

run: all
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -serial null -serial stdio

debug: all
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -S -s

directories: $(OUT_DIR)

$(OUT_DIR):
	mkdir -p $(OUT_DIR)

clean:
	rm -f out/* kernel8.*
```




#### Mailbox
* [The mailbox](https://bitbanged.com/posts/understanding-rpi/the-mailbox/)
* [Mailbox property interface](https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface#get-board-mac-address)
![截圖 2024-02-29 上午11.17.12](https://hackmd.io/_uploads/SyErDdpnp.png)
* The mailboxes act as pipes between the CPU and the GPU. The peripheral exposes two mailboxes, 0 and 1, to allow full-duplex communication. Each mailbox operates as a FIFO buffer with the capacity to store up to eight 32-bit words. Notably, mailbox 0 alone supports triggering interrupts on the CPU side, which makes it well-suited for CPU-initiated read operations. As a result, CPI-initiated write operations naturally get channelled through mailbox 1.
* 簡單來說，把mbox的表格填好，就會在指定位置拿到回傳的資料。==表中0x0c與0x10填反了。==
![截圖 2024-02-29 上午11.05.07](https://hackmd.io/_uploads/SJXOEO6na.png)
##### mbox[0]
* 填這個mbox的大小
##### mbox[1]
* 填status
![截圖 2024-02-29 上午11.08.38](https://hackmd.io/_uploads/SkgHBu63a.png)
##### mbox[2]
* [根據你要的功能填tag ID](https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface#get-board-mac-address)
![截圖 2024-02-28 上午11.48.15](https://hackmd.io/_uploads/Byi-a7nnT.png =200x200)
![截圖 2024-02-28 上午11.49.46](https://hackmd.io/_uploads/H1SPT7hnp.png =200x200)
##### mbox[3]
* 填tag buffer的大小
##### mbox[4]
* 填Tag status，只關心第31個bit
![截圖 2024-02-29 上午11.11.23](https://hackmd.io/_uploads/rk81LdThT.png)
##### mbox[5]、mbox[6]
* value buffer，請求前不用填，回覆後值會放這裡
##### mbox[6]
* Null tag to mark end of the tag list.
##### mbox_call(unsigned int* mbox, unsigned char channel)
* status
![截圖 2024-02-29 上午11.19.51](https://hackmd.io/_uploads/SJLJ_OT3T.png)
    * 1000 0000 0000 0000 0000 0000 0000 0000 = x80000000
    * 0100 0000 0000 0000 0000 0000 0000 0000 = x40000000
* Message status
![截圖 2024-02-29 上午11.25.28](https://hackmd.io/_uploads/ryQEKua3p.png)

```=C
#define MBOX_BASE   (MMIO_BASE + 0xb880)
// address map
#define MBOX_READ   (unsigned int*)(MBOX_BASE)
#define MBOX_STATUS (unsigned int*)(MBOX_BASE + 0x18)
#define MBOX_WRITE  (unsigned int*)(MBOX_BASE + 0x20)
// flag
#define MBOX_EMPTY  0x40000000
#define MBOX_FULL   0x80000000
// code
#define MBOX_CODE_BUF_REQ               0x00000000
#define MBOX_CODE_BUF_RES_SUCC          0x80000000
#define MBOX_CODE_TAG_REQ               0x00000000
// tag
#define MBOX_TAG_GET_BOARD_REVISION     0x00010002
#define MBOX_TAG_GET_ARM_MEMORY         0x00010005
#endif
```
```=C
int mbox_call(unsigned int* mbox, unsigned char channel) {
    // create 28 bit data + 4 bit channel
    // r is an address
    unsigned int r = (unsigned int)(((unsigned long)mbox) & (~0xF)) | (channel & 0xF);
    // wait until full flag unset
    // MBOX_STATUS : 0x18
    // MBX_FULL : Status register bits  
    // bit 31 :	Mailbox full 
    while (*MBOX_STATUS & MBOX_FULL) {
    }
    // write address of message + channel to mailbox
    // write to mbox1
    *MBOX_WRITE = r;
    // wait until response
    while (1) {
        // wait until empty flag unset
        while (*MBOX_STATUS & MBOX_EMPTY) {
        }
        // is it a response to our msg?
        if (r == *MBOX_READ) {
            // check is response success
            return mbox[1] == MBOX_CODE_BUF_RES_SUCC;
        }
    }
    return 0;
}
```





















