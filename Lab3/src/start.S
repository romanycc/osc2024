#include "sysregs.h"
#include "exception_table.h"
.section ".text.boot"

.global _start

_start:
    // get cpu id
    mrs     x1, MPIDR_EL1
    and     x1, x1, #3
    cbz     x1, 2f
    // if cpu_id > 0, stop
1:
    wfe
    b       1b
    // if cpu_id == 0
2:
    // set stack pointer
    ldr     x1, =_start
    mov     sp, x1

    // clear bss
    ldr     x1, =__bss_start
    ldr     x2, =__bss_size
3:  cbz     x2, 4f
    str     xzr, [x1], #8
    sub     x2, x2, #1
    cbnz    x2, 3b
4:
    // disable MMU
    // msr : Load specified fields of the CPSR or SPSR with an immediate constant, or from the contents of a general-purpose register.
    ldr     x1, =SCTLR_VALUE_MMU_DISABLED
    msr     sctlr_el1, x1

    // make el0, el1 can use Floating point and Advanced SIMD
    ldr     x1, =CPACR_EL1_VALUE
    msr     CPACR_EL1, x1

    // set AArch64 for el2
    ldr     x1, =HCR_EL2_VALUE
    msr     hcr_el2, x1

    // mask all interrupt, and set interrupt level to el1h
    ldr     x1, =SPSR_EL2_VALUE
    msr     spsr_el2, x1
    // The ADR instruction loads an address within a certain range, without performing a data load.
    adr     x1, el1_start
    msr     elr_el2, x1

    // for CurrentEL
    mrs     x26, CurrentEL

    eret

// Rpi3’s CPU runs in EL2 after being booted by default, but we want the kernel to run in EL1. 
// Hence, your kernel needs to switch to EL1 at the beginning.
el1_start:
    // set stack pointer
    ldr     x1, =0x60000
    mov     sp, x1

    // load exception_table to VBAR_EL1
    adr     x1, exception_table
    msr     vbar_el1, x1

    // mask all interrupt, and set exception level to el0
    ldr     x1, =SPSR_EL1_VALUE
    msr     spsr_el1, x1

    adr     x1, el0_start
    msr     elr_el1, x1
    
    // for CurrentEL
    mrs     x27, CurrentEL

    eret

el0_start:
    // set stack pointer
    ldr     x3, =0x40000
    mov     sp, x3
    // for cpio, command this line if using qemu
    mov     x0, x25
    // for print CurrentEL
    mov     x1, x26
    mov     x2, x27
    // jump to main function in C
    bl      main
    // halt this core if return
    b       1b
