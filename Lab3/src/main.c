#include "shell.h"
#include "uart.h"
#include "heap.h"
#include "dtb.h"
#include "initramfs.h"
#define CMD_LEN 128
#include <stdint.h>

enum shell_status{
    Read,
    Parse
};

int main(void *dtb_location, uint32_t CurrentEL2, uint32_t CurrentEL1){
    shell_init();
    heap_init();
    uart_printf("malloc test: 0x%x\n", malloc(8));
    uart_printf("malloc test: 0x%x\n", malloc(8));
    uart_printf("malloc test: 0x%x\n", malloc(8));
    //fdt_dump(dtb_location);
    uart_printf("initrams(before): %x\n", initrd_getLo());
    fdt_find_do(dtb_location, "linux,initrd-start", initrd_fdt_callback);
    uart_printf("initrams(after): %x\n", initrd_getLo());
    // print CurrentEL
    uart_printf("CurrentEL2: %d\n", CurrentEL2/4);
    uart_printf("CurrentEL1: %d\n", CurrentEL1/4);
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