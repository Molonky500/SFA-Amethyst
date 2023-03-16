#include <asm.h>

	.text
	.section .init

	# crt0.s file for the GameCube V1.1 by Costis (costis@gbaemu.com)!
	#
	# Updates: Added support for clearing the BSS section so that global
	#	     variables are cleared to 0 upon start-up.
	#
	# This is start-up code for initializing the GameCube system and hardware
	# before executing the actual user program code. It clears the GPR's,
	# initializes the FPR's, initializes the Data, Code, and L2 caches, clears
	# and initializes SPR's, and disables exceptions (interrupts).
	#
	# Have fun!!! Please e-mail any suggestions or bugs to costis@gbaemu.com.

	# modified by Rena 2022 Sep 28:
	# - set/use r2 and r13 how the game wants

	# Entry Point

	.extern __InitBATS
	.extern __InitPS
	.extern __InitFPRS
	.extern __InitCache
	.extern __InitSystem
	.extern __sbss_start, __bss_end, __CheckARGV
	.globl _start, __main, __system_argv
_start:
	b	startup
	.long	0x5f617267 # arg magic
__argv:
	.long 0 # argv magic
	.long 0 # command line
	.long 0 # command line length
	.long 0 # argc
	.long 0 # argv
	.long 0 # end address of argv

startup:
	//bl		__InitBATS     # Initialize BATs to a clear and known state
	//bl		__InitGPRS     # Initialize the General Purpose Registers
	//bl		__InitHardware # Initialize some aspects of the Hardware
	//bl		__InitSystem   # Initialize more cache aspects, clear a few SPR's, and disable interrupts.
	//bl		__CheckARGV    # check for argv & initialise struct

	lis		r3,__isIPL@h
	ori		r3,r3,__isIPL@l
	cmplwi	r3,0
	bne		1f

	# Clear the SBSS section!
	lis		r3,__sbss_start@h
	ori		r3,r3,__sbss_start@l
	li		r4,0
	lis		r5,__sbss_end@h
	ori		r5,r5,__sbss_end@l
	sub		r5,r5,r3
	bl		_memset

	# Clear the BSS section!
	lis		r3,__bss_start@h
	ori		r3,r3,__bss_start@l
	li		r4,0
	lis		r5,__bss_end@h
	ori		r5,r5,__bss_end@l
	sub		r5,r5,r3
	bl		_memset

1:
	lis     r20, 0xCD80 //IPC reg
	lis     r4,  0xFF00
	ori     r4,  r4,  0xFFFF
	lwz     r3,  0xC0(r20)
	and     r3,  r3,  r4
	oris    r3,  r3,  0x01
	stw     r3,  0xC0(r20) //write debug port

	b      SYS_Init # Branch to the user code!

	.section .bss
	.balign 8
.globl __crt0stack_end
.globl __crt0stack
__crt0stack_end:
	.space 0x4000
__crt0stack:

	.globl    __system_argv
	.section  .sdata,"aw",@progbits
	.align    2
	.type     __system_argv, @object
	.size     __system_argv, 4
__system_argv:
	.long __argv
