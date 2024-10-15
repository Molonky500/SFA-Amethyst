#include <asm.h>

.section .text,"x"
.globl WriteDebugPort
.extern __InitBATS
.extern __InitPS
.extern __InitFPRS
.extern __InitCache
.extern __InitSystem
.extern __sbss_start, __bss_end, __CheckARGV
.globl _start, __main, __system_argv
.globl _start_hack

//XXX some of these are redundant
.global get_r1
.global get_r2
.global get_r13
.global set_r1
.global set_r2
.global set_r13
.global get_msr
.global set_msr
.global get_dar

/** This is the application's "bootloader".
 *  This is a hideous hack to work around HBC refusing to load
 *  apps that have sections at certain addresses.
 *  The build script appends the "real" DOL to this one and
 *  updates the section headers to load that DOL to a specific
 *  RAM address.
 *  This program just copies that DOL to the address it should
 *  be loaded at and jumps to it.
 *  This needs to be in this file instead of a file of its own
 *  because reasons.
 */

#define DOL_NUM_TEXT_SECTIONS 7
#define DOL_NUM_DATA_SECTIONS 11
#define DOL_SECTION_SIZE ((DOL_NUM_TEXT_SECTIONS+DOL_NUM_DATA_SECTIONS)*4)

_start_hack:
	b startup2
	.long	0x5f617267 # magic
__argv: //XXX use this
	.long 0 # argv magic
	.long 0 # command line
	.long 0 # command line length
	.long 0 # argc
	.long 0 # argv
	.long 0 # end address of argv

startup2:
	bl		__InitGPRS     # Initialize the General Purpose Registers

	bl .getpc
	.getpc:
		mflr r14

	//set up debug port
	lis     r20, 0xCD80 //IPC reg
	lis     r3,  0xFFFF
	ori     r3,  r3,  0xFFFF
	stw     r3,  0x64(r20) //AHBPROT: access everything
	lis     r4,  0x00FF
	ori     r4,  r4,  0x0020
	lwz     r5,  0xFC(r20)
	or      r5,  r5,  r4
	stw     r5,  0xFC(r20) //allow PPC access
	stw     r3,  0xDC(r20) //enable GPIOs
	lwz     r5,  0xC4(r20)
	or      r5,  r5,  r4
	stw     r5,  0xC4(r20) //set directions

	li      r3,  0x69
	bl      WriteDebugPort

	//bl		__InitBATS     # Initialize BATs to a clear and known state
	//li      r3,  0xB9
	//bl      WriteDebugPort

	bl		__InitHardware # Initialize some aspects of the Hardware
	li      r3,  0x6A
	bl      WriteDebugPort

	bl		__InitSystem   # Initialize more cache aspects, clear a few SPR's, and disable interrupts.
	li      r3,  0x6B
	bl      WriteDebugPort

	//load DOL
	li      r0,  0 //I don't *think* this is needed for cache ops?
	lis     r15, 0x8051 //src addr

    //get BSS range and zero it
    addi    r14, r15, (DOL_SECTION_SIZE * 3)
    lwz     r3,  0(r14) //addr
    lwz     r4,  4(r14) //size
	//lis     r5,  0x7FFF
	//ori     r5,  r5,  0xFFFF
	//and     r3,  r3,  r5 //clear high bit of address
	/*bl      DCZeroRange
    lwz     r3,  0(r14) //addr
    lwz     r4,  4(r14) //size
	bl      ICInvalidateRange*/

	li      r6,  0
	srwi    r4,  r4,  2
	mtctr   r4
1:
	stw     r6,  4(r3)
	dcbst   r0,  r3
	icbi    r0,  r3 //more than needed but oh well
	addi    r3,  r3,  4
	bdnz    1b

    //copy sections to appropriate places
    li      r16, DOL_NUM_TEXT_SECTIONS + DOL_NUM_DATA_SECTIONS //count
    //subi    r17, r15,  4 //src addr
	mr      r17, r15
.nextSection:
    //check count
    cmpwi   r16, 0
    beq     .blastoff
    subi    r16, r16, 1

    //read section
    lwz     r4,  0(r17) // get offset
    lwz     r5,  DOL_SECTION_SIZE(r17) // get address
    lwz     r6,  (DOL_SECTION_SIZE*2)(r17) // get size
	addi    r17, r17, 4
    cmpwi   r6,  0 //size is zero?
    beq     .nextSection
	//lis     r7,  0x7FFF
	//ori     r7,  r7,  0xFFFF
	//and     r5,  r5,  r7 //clear high bit of address

    //adjust for lwzu
    subi    r4,  r4,  4
    subi    r5,  r5,  4
    add     r4,  r4,  r15

    //copy section
.copy:
    lwzu    r8,  4(r4)
    stwu    r8,  4(r5)
    subi    r6,  r6,  4
	dcbst   r0,  r5
	icbi    r0,  r5 //more than needed but oh well
    cmpwi   r6,  0
    bne     .copy

    //flush caches
    /*lwz     r3,  DOL_SECTION_SIZE(r17) // get address
    lwz     r4,  (DOL_SECTION_SIZE*2)(r17) // get size
    bl      DCFlushRange
    lwz     r3,  DOL_SECTION_SIZE(r17) // get address
    lwz     r4,  (DOL_SECTION_SIZE*2)(r17) // get size
    bl      ICInvalidateRange*/

    b .nextSection

.blastoff:
	sync
	isync
	//bl		__InitBATS     # Initialize BATs to a clear and known state

	li      r3,  0x42
	bl      WriteDebugPort

	//jump to entry point (which is _start, not _start_hack)
    lwz     r3,  8(r14) //entry point
	//lis     r4,  0x7FFF
	//ori     r4,  r4,  0xFFFF
	//and     r3,  r3,  r4
    mtctr   r3
    bctr

WriteDebugPort:
	/*lis     r5,  0xCD80 //IPC reg
	lis     r4,  0xFF00
	ori     r4,  r4,  0xFFFF
	lwz     r6,  0xC0(r5)
	and     r6,  r6,  r4
	slwi    r3,  r3,  16
	or      r6,  r6,  r3
	stw     r6,  0xC0(r5) //write debug port
	mfctr   r4
	lis     r3,  0x0005
.delay1:
	bdnz    .delay1
	xoris   r6,  r6,  0xFF //invert debug port bits
	stw     r6,  0xC0(r5) //write debug port
	lis     r3,  0x0005
	mtctr   r3
.delay2:
	bdnz    .delay2
	mtctr   r4*/
	blr

__InitGPRS:
	# Clear all of the GPR's to 0
	li      r0,0
	li      r3,0
	li      r4,0
	li      r5,0
	li      r6,0
	li      r7,0
	li      r8,0
	li      r9,0
	li      r10,0
	li      r11,0
	li      r12,0
	li      r14,0
	li      r15,0
	li      r16,0
	li      r17,0
	li      r18,0
	li      r19,0
	li      r20,0
	li      r21,0
	li      r22,0
	li      r23,0
	li      r24,0
	li      r25,0
	li      r26,0
	li      r27,0
	li      r28,0
	li      r29,0
	li      r30,0
	li      r31,0

	lis		sp,__crt0stack@h
	ori		sp,sp,__crt0stack@l
	addi	sp,sp,-4
	stw		r0,0(sp)
	stwu	sp,-56(sp)

	//load r2 and r13 with values the game uses.
	lis     r2,  0x803E
	ori     r2,  r2,  0x6500
	lis     r13, 0x803E
	ori     r13, r13, 0x31E0
	blr

__InitHardware:
	# Enable the Floating Point Registers
	mfmsr   r3
	ori     r3,r3,MSR_FP
	mtmsr   r3

	mflr	r31
	bl		__InitPS 			# Initialize Paired Singles
	bl		__InitFPRS		 	# Initialize the FPR's
	bl		__InitCache 		# Initialize the system caches
	mtlr	r31
	blr

	//r3 = ptr, r4 = fill, r5 = size
	.globl _memset
_memset:
	clrlwi.	r6,r5,29
	srwi	r5,r5,2
	subi	r3,r3,4
	mtctr	r5
1:	stwu	r4,4(r3)
	bdnz	1b
	cmplwi	r6,0
	beq		3f
2:	stbu	r4,1(r3)
	addic.	r6,r6,-1
	bne+	2b
3:	blr


get_r1:
    mr r3, r1
    blr
get_r2:
    mr r3, r2
    blr
get_r13:
    mr r3, r13
    blr

set_r1:
    mr r1, r3
    blr
set_r2:
    mr r2, r3
    blr
set_r13:
    mr r13, r3
    blr

get_msr:
    mfmsr r3
    blr
set_msr:
    mtmsr r3
    blr
get_dar:
    mfspr r3, 19
    blr

	.globl __realmode
__realmode:
	clrlwi	r3,r3,2
	mtsrr0	r3
	mfmsr	r3
	rlwinm	r3,r3,0,28,25
	mtsrr1	r3
	mflr    r29
	li      r3,  0x77
	bl      WriteDebugPort
	mtlr    r29
	rfi

	// complete init sequence taken from bootmii's ppc skeleton. thanks to segher
	// after a talk with dhewg we came to that point that it's good to wipe+setup BATS correctly
	.globl __configBATS
__configBATS:
	mflr    r29
	li      r3,  0xC1
	bl      WriteDebugPort
	// HID0 = 00110c64:
	// bus checkstops off, sleep modes off,
	// caches off, caches invalidate,
	// store gathering off, enable data cache
	// flush assist, enable branch target cache,
	// enable branch history table
	lis		r3,0x0011
	ori		r3,r3,0x0c64
	mtspr	HID0,r3
	isync

#if defined(HW_RVL)
	lis		r3,0x8200		//bits set: H4A(HID4 access), SBE(2nd BAT enabled)
	mtspr	HID4,r3
	isync
#endif

	// clear all BATs
	li		r0,0
	mtspr	IBAT0U,r0; mtspr	IBAT1U,r0; mtspr	IBAT2U,r0; mtspr	IBAT3U,r0		// IBAT0...3
	mtspr	DBAT0U,r0; mtspr	DBAT1U,r0; mtspr	DBAT2U,r0; mtspr	DBAT3U,r0		// DBAT0...3
#if defined(HW_RVL)
	mtspr	IBAT4U,r0; mtspr	IBAT5U,r0; mtspr	IBAT6U,r0; mtspr	IBAT7U,r0		// IBAT4...7
	mtspr	DBAT4U,r0; mtspr	DBAT5U,r0; mtspr	DBAT6U,r0; mtspr	DBAT7U,r0		// DBAT4...7
#endif
	isync

	// clear all SRs
	lis		r0,0x8000
	mtsr	0,r0; mtsr	1,r0; mtsr	2,r0; mtsr	3,r0; mtsr	4,r0; mtsr	5,r0; mtsr	6,r0
	mtsr	7,r0; mtsr	8,r0; mtsr	9,r0; mtsr	10,r0; mtsr	11,r0; mtsr	12,r0; mtsr	13,r0
	mtsr	14,r0; mtsr	15,r0
	isync

	// set [DI]BAT0 for 256MB@80000000,
	// real 00000000, WIMG=0000, R/W
	li		r3,2
	lis		r4,0x8000
	ori		r4,r4,0x1fff
	mtspr	IBAT0L,r3
	mtspr	IBAT0U,r4
	mtspr	DBAT0L,r3
	mtspr	DBAT0U,r4
	isync

#if defined(HW_RVL)
	// set [DI]BAT4 for 256MB@90000000,
	// real 10000000, WIMG=0000, R/W
	addis	r3,r3,0x1000
	addis	r4,r4,0x1000
	mtspr	IBAT4L,r3
	mtspr	IBAT4U,r4
	mtspr	DBAT4L,r3
	mtspr	DBAT4U,r4
	isync
#endif

	// set DBAT1 for 256MB@c0000000,
	// real 00000000, WIMG=0101, R/W
	li		r3,0x2a
	lis		r4,0xc000
	ori		r4,r4,0x1fff
	mtspr	DBAT1L,r3
	mtspr	DBAT1U,r4
	isync

#if defined(HW_RVL)
	// set DBAT5 for 256MB@d0000000,
	// real 10000000, WIMG=0101, R/W
	addis	r3,r3,0x1000
	addis	r4,r4,0x1000
	mtspr	DBAT5L,r3
	mtspr	DBAT5U,r4
	isync
#endif

	mfmsr	r3
	ori		r3,r3,MSR_DR|MSR_IR
	mtsrr1	r3
	mflr	r3
	oris    r3,r3,0x8000
	mtsrr0	r3
	li      r3,  0xCF
	bl      WriteDebugPort
	mtlr    r29
	rfi

	.globl __InitFPRS
__InitFPRS:
	# Enable the Floating Point Registers
	mfmsr   r3
	ori     r3,r3,MSR_FP
	mtmsr   r3
	mfspr	r3,920
	extrwi.	r3,r3,1,2
	beq		1f

	# Clear all of the PS FPR's to 0
	lis		r3,zeroPS@ha
	addi	r3,r3,zeroPS@l
	psq_l	fr0,0(r3),0,0
	ps_mr	fr1,fr0
	ps_mr	fr2,fr0
	ps_mr	fr3,fr0
	ps_mr	fr4,fr0
	ps_mr	fr5,fr0
	ps_mr	fr6,fr0
	ps_mr	fr7,fr0
	ps_mr	fr8,fr0
	ps_mr	fr9,fr0
	ps_mr	fr10,fr0
	ps_mr	fr11,fr0
	ps_mr	fr12,fr0
	ps_mr	fr13,fr0
	ps_mr	fr14,fr0
	ps_mr	fr15,fr0
	ps_mr	fr16,fr0
	ps_mr	fr17,fr0
	ps_mr	fr18,fr0
	ps_mr	fr19,fr0
	ps_mr	fr20,fr0
	ps_mr	fr21,fr0
	ps_mr	fr22,fr0
	ps_mr	fr23,fr0
	ps_mr	fr24,fr0
	ps_mr	fr25,fr0
	ps_mr	fr26,fr0
	ps_mr	fr27,fr0
	ps_mr	fr28,fr0
	ps_mr	fr29,fr0
	ps_mr	fr30,fr0
	ps_mr	fr31,fr0

	# Clear all of the FPR's to 0
1:	lis	    r3,zeroF@ha
	lfd	    fr0,zeroF@l(r3)
	fmr     fr1,fr0
	fmr     fr2,fr0
	fmr     fr3,fr0
	fmr     fr4,fr0
	fmr     fr5,fr0
	fmr     fr6,fr0
	fmr     fr7,fr0
	fmr     fr8,fr0
	fmr     fr9,fr0
	fmr     fr10,fr0
	fmr     fr11,fr0
	fmr     fr12,fr0
	fmr     fr13,fr0
	fmr     fr14,fr0
	fmr     fr15,fr0
	fmr     fr16,fr0
	fmr     fr17,fr0
	fmr     fr18,fr0
	fmr     fr19,fr0
	fmr     fr20,fr0
	fmr     fr21,fr0
	fmr     fr22,fr0
	fmr     fr23,fr0
	fmr     fr24,fr0
	fmr     fr25,fr0
	fmr     fr26,fr0
	fmr     fr27,fr0
	fmr     fr28,fr0
	fmr     fr29,fr0
	fmr     fr30,fr0
	fmr     fr31,fr0
	mtfsf   255,fr0

	# Return
	blr

	.extern ICFlashInvalidate
	.globl __InitPS
__InitPS:
	mflr	r0
	stw		r0,4(sp)
	stwu	sp,-8(sp)
	mfspr	r3,HID2
	oris	r3,r3,0xA000
	mtspr	HID2,r3
	isync
	bl		ICFlashInvalidate
	sync
	li		r3,0
	mtspr	GQR0,r3
	mtspr	GQR1,r3
	mtspr	GQR2,r3
	mtspr	GQR3,r3
	mtspr	GQR4,r3
	mtspr	GQR5,r3
	mtspr	GQR6,r3
	mtspr	GQR7,r3
	isync
	lwz		r0,12(sp)
	addi	sp,sp,8
	mtlr	r0
	blr

	.extern ICEnable
	.extern DCEnable
	.extern L2Init
	.extern L2Enable
	.globl __InitCache
__InitCache:
	mflr    r0
	stw     r0, 4(sp)
	stwu    sp, -16(sp)
	stw     r31, 12(sp)

	mfspr   r3,HID0
	rlwinm. r0,r3, 0, 16, 16	// Check if the Instruction Cache has been enabled or not.
	bne     ICEnabled

	bl		ICEnable
ICEnabled:
	mfspr   r3, HID0
	rlwinm. r0, r3, 0, 17, 17	// Check if the Data Cache has been enabled or not.
	bne     DCEnabled

	bl		DCEnable
DCEnabled:

	mfspr   r3, L2CR
	clrrwi. r0, r3, 31			// Check if the Locked Cache has been enabled or not.
	bne     L2Enabled

	bl		L2Init
	bl		L2Enable

L2Enabled:
	# Restore the non-volatile registers to their previous values and return.
	lwz     r0, 20(sp)
	lwz     r31, 12(sp)
	addi    sp, sp, 16
	mtlr    r0
	blr

	.globl __InitSystem
__InitSystem:
	mflr    r0
	stw     r0, 4(sp)
	stwu    sp, -24(sp)
	stmw	r29, 12(sp)

	# Disable interrupts!
	mfmsr   r3
	rlwinm  r4,r3,0,17,15
	rlwinm  r4,r4,0,26,24
	mtmsr   r4

	# Clear various SPR's
	li      r3,0
	mtspr   952, r3
	mtspr   956, r3
	mtspr   953, r3
	mtspr   954, r3
	mtspr   957, r3
	mtspr   958, r3

	mtspr   912, r3 # GQR0
	mtspr   913, r3 # GQR1

	lis     r3,  0x0004
	ori     r3,  r3,  0x0004
	mtspr   914, r3 # GQR2

	lis     r3,  0x0005
	ori     r3,  r3,  0x0005
	mtspr   915, r3 # GQR3

	lis     r3,  0x0006
	ori     r3,  r3,  0x0006
	mtspr   916, r3 # GQR4

	lis     r3,  0x0007
	ori     r3,  r3,  0x0007
	mtspr   917, r3 # GQR5

	lis     r3,  0x0507
	ori     r3,  r3,  0x0507
	mtspr   918, r3 # GQR6

	lis     r3,  0x0807
	ori     r3,  r3,  0x0807
	mtspr   919, r3 # GQR7
	isync

#if defined(HW_RVL)
	mfspr	r3,HID4
	oris	r3,r3,0x0190		//set additional bits in HID4: SR0(store 0), LPE(PS LE exception), L2CFI(L2 castout prior to L2 inv. flash)
	mtspr	HID4,r3
	isync
#endif

	# Disable Speculative Bus Accesses to non-guarded space from both caches.
	mfspr   r3, HID0
	ori     r3, r3, 0x0200
	mtspr   HID0, r3
	isync

	# Set the Non-IEEE mode in the FPSCR
	mtfsb1  29

	# Disable Write Gather Pipe
	mfspr   r3,HID2 # (HID2)
	rlwinm  r3, r3, 0, 2, 0
	mtspr   HID2,r3 # (HID2)
	isync

	# Restore the non-volatile registers to their previous values and return.
	lwz     r0, 28(sp)
	lmw	r29,12(sp)
	addi    sp, sp, 24
	mtlr    r0
	blr

	.globl __flush_cache
__flush_cache:
	lis		r5,0xffff
	ori		r5,r5,0xfff1
	and		r5,r5,r3
	subf	r3,r5,r3
	add		r4,r4,r3
1:	dcbst	r0,r5
	sync
	icbi	r0,r5
	addic	r5,r5,8
	subic.	r4,r4,8
	bge		1b
	sync
	blr

	.globl __reset
__reset:
	b		1f
9:	mfspr	r8,HID0
	ori		r8,r8,0x0008
	mtspr	HID0,r8
	isync
	sync
	nop
	b		2f
1:	b		3f
2:	mftb	r5
4:	mftb	r6
	subf	r7,r5,r6
	cmplwi	r7,0x1124
	blt		4b
	nop
	b		5f
3:	b		6f
5:	lis		r8,0xCC00
	ori		r8,r8,0x3000
	li		r4,3
	stw		r4,36(r8)
	stw		r3,36(r8)
	nop
	b		7f
6:	b		8f
7:	nop
	b		7b
8:	b		9b

	.globl __InitBATS
__InitBATS:
	mflr	r31
	oris	r31,r31,0x8000
	li      r3,  0xB1
	bl      WriteDebugPort
	lis		r3,__configBATS@h
	ori		r3,r3,__configBATS@l
	bl		__realmode
	li      r3,  0xB2
	bl      WriteDebugPort
	mtlr	r31
	blr

	.globl SYS_SwitchFiber
SYS_SwitchFiber:
	mflr	r0
	mr		r9,sp
	stwu	r9,-8(r8)
	mr		sp,r8
	stw		r0,4(r9)
	mtlr	r7
	blrl
	lwz		r5,0(sp)
	lwz		r0,4(r5)
	mtlr	r0
	mr		sp,r5
	blr

	.section .data
	.balign	4
zeroF:
	.double 0.0
zeroPS:
	.float 0.0,0.0
