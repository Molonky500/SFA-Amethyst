#ifndef __ASMINLINE_H__
#define __ASMINLINE_H__

//Prologue macro for naked functions
//Saves r0 and r3-r31, then clobbers r0
//The purpose of using this is that r2-r31 are unchanged, and that
//r3-r31 are saved to the stack (not just r14-r31), so it can be used
//to hook into the middle of a function.
#define ASM_FUNC_START(stackSize) \
    "stwu    1, -" #stackSize      "(1) \n" \
    "stw     0,                 0x10(1) \n" \
    "stmw    3,                 0x14(1) \n" \
    "mflr    0                          \n" \
    "stw     0,  (" #stackSize  "+4)(1) \n"

#define ASM_FUNC_END(stackSize) \
    "lwz     0,                0x10(1) \n" \
    "lwz     3, (" #stackSize  "+4)(1) \n" \
    "mtlr    3                         \n" \
    "lmw     3,                0x14(1) \n" \
    "addi    1, 1, " #stackSize       "\n"

//same as above, but doesn't preserve r3, for return values.
#define ASM_RFUNC_START(stackSize) \
    "stwu    1, -" #stackSize      "(1) \n" \
    "stw     0,                 0x10(1) \n" \
    "stmw    4,                 0x14(1) \n" \
    "mflr    0                          \n" \
    "stw     0,  (" #stackSize  "+4)(1) \n"

#define ASM_RFUNC_END(stackSize) \
    "lwz     0,                0x10(1) \n" \
    "lwz     4, (" #stackSize  "+4)(1) \n" \
    "mtlr    4                         \n" \
    "lmw     4,                0x14(1) \n" \
    "addi    1, 1, " #stackSize       "\n"

#endif //__ASMINLINE_H__
