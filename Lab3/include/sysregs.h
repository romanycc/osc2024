#ifndef __SYSREGS_H__
#define __SYSREGS_H__

// SCTLR_EL1, System Control Register (EL1)
// Close MMU

#define SCTLR_VALUE_MMU_DISABLED 0

// HCR_EL2, Hypervisor Configuration Register
// Set AARCH64

#define HCR_EL2_RW_AARCH64  (1 << 31)
#define HCR_EL2_VALUE       (HCR_EL2_RW_AARCH64)

// SPSR_EL2, Saved Program Status Register (EL2)

#define SPSR_EL2_MASK_ALL   (0b1111 << 6)   // set DAIF = 1 
#define SPSR_EL2_EL1h       (0b0101 << 0)   // AArch64 Exception level and selected Stack Pointer.EL1 with SP_EL1 (EL1h)
#define SPSR_EL2_VALUE      (SPSR_EL2_MASK_ALL | SPSR_EL2_EL1h)

// SPSR_EL1, Saved Program Status Register (EL1)

#define SPSR_EL1_MASK       (0b0000 << 6)   // set DAIF = 1 
#define SPSR_EL1_EL0        (0b0000 << 0)   // AArch64 Exception level and selected Stack Pointer.EL0t.
#define SPSR_EL1_VALUE      (SPSR_EL1_MASK | SPSR_EL1_EL0)

// CPACR_EL1, Architectural Feature Access Control Register
// FPEN

#define CPACR_EL1_FPEN      (0b11 << 20)
#define CPACR_EL1_VALUE     (CPACR_EL1_FPEN)

#endif