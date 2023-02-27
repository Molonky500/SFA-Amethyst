#include "asm.h"

.text
.section .init

/** This is the application's "bootloader".
 *  This is a hideous hack to work around HBC refusing to load
 *  apps that have sections at certain addresses.
 *  The build script appends the "real" DOL to this one and
 *  updates the section headers to load that DOL to a specific
 *  RAM address.
 *  This program just copies that DOL to the address it should
 *  be loaded at and jumps to it.
 */

#define DOL_NUM_TEXT_SECTIONS 7
#define DOL_NUM_DATA_SECTIONS 11
#define DOL_SECTION_SIZE ((DOL_NUM_TEXT_SECTIONS+DOL_NUM_DATA_SECTIONS)*4)

.globl SYS_Init
SYS_Init:
    lis     r15, 0x8010 //src addr

    //get BSS range and zero it
    addi    r14, r15, (DOL_SECTION_SIZE * 3)
    lwz     r3,  0(r14) //addr
    lwz     r4,  4(r14) //size
    bl      DCZeroRange
    lwz     r3,  0(r14) //addr
    lwz     r4,  4(r14) //size
    bl      ICInvalidateRange

    //copy sections to appropriate places
    li      r16, DOL_NUM_TEXT_SECTIONS + DOL_NUM_DATA_SECTIONS //count
    subi    r17, r15,  4 //src addr
.nextSection:
    //check count
    cmpwi   r16, 0
    beq     .blastoff
    subi    r16, r16, 1

    //read section
    lwzu    r4,  4(r17) // get offset
    lwz     r5,  DOL_SECTION_SIZE(r17) // get address
    lwz     r6,  (DOL_SECTION_SIZE*2)(r17) // get size
    cmpwi   r6,  0 //size is zero?
    beq     .nextSection

    //adjust for lwzu
    subi    r4,  r4,  4
    subi    r5,  r5,  4
    add     r4,  r4,  r15

    //copy section
.copy:
    lwzu    r8,  4(r4)
    stwu    r8,  4(r5)
    subi    r6,  r6,  4
    cmpwi   r6,  0
    bne     .copy

    //flush caches
    lwz     r3,  DOL_SECTION_SIZE(r17) // get address
    lwz     r4,  (DOL_SECTION_SIZE*2)(r17) // get size
    bl      DCFlushRange
    lwz     r3,  DOL_SECTION_SIZE(r17) // get address
    lwz     r4,  (DOL_SECTION_SIZE*2)(r17) // get size
    bl      ICInvalidateRange

    b .nextSection

.blastoff:
    //jump to entry point
    lwz     r3,  8(r14) //entry point
    mtctr   r3
    bctr
