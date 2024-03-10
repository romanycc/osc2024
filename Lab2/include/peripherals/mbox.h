#include "../mmio.h"

#ifndef MBOX_H
#define MBOX_H 

#define MBOX_BASE                       (MMIO_BASE + 0xb880)

// address map
#define MBOX_READ                       (unsigned int*)(MBOX_BASE)
#define MBOX_STATUS                     (unsigned int*)(MBOX_BASE + 0x18)
#define MBOX_WRITE                      (unsigned int*)(MBOX_BASE + 0x20)

// flag
#define MBOX_EMPTY                      0x40000000
#define MBOX_FULL                       0x80000000

// code
#define MBOX_CODE_BUF_REQ               0x00000000
#define MBOX_CODE_BUF_RES_SUCC          0x80000000
#define MBOX_CODE_TAG_REQ               0x00000000

// tag
#define MBOX_TAG_GET_BOARD_REVISION     0x00010002
#define MBOX_TAG_GET_ARM_MEMORY         0x00010005
#endif