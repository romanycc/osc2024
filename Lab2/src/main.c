#include "shell.h"
#include "uart.h"
#include "heap.h"
#define CMD_LEN 128

enum shell_status{
    Read,
    Parse
};

int main(){
    shell_init();
    heap_init();
    uart_printf("malloc test: 0x%x\n", malloc(8));
    uart_printf("malloc test: 0x%x\n", malloc(8));
    uart_printf("malloc test: 0x%x\n", malloc(8));
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