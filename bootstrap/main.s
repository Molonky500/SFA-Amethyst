# Contents of boot.bin.
# This file is loaded at startup by our modified main.dol and executed, as a
# sort of "second-stage bootloader". It must be position-independent code.
# The build process appends the other patches' binary to this file's, and fills
# in the header to tell it how to load that code.
# It exppects r3 to be the address it's loaded at and r4 to be the size
# of boot.bin.

.set DEBUG,1
.text
.include "common.s"

constants:
    .set STACK_SIZE,   0x100 # how much to reserve
    .set SP_LR_SAVE,   STACK_SIZE+4
    .set SP_TMP1,      0x10
    .set SP_PATCH_ADDR,0x14
    .set SP_FLAGS_ADDR,0x18
    .set SP_GPR_SAVE,  0x1C

_start:
    # entry point of this file.
    # we patch the game's entry point to this as well
    # so that on Wii we can make up for the init code
    # being truncated (since it would overlap with
    # RevolutionOS globals)
    # and so that we can grab the args.
    b      _start2

    .long	0x5f617267 # magic to tell the loader to put args here.
__argv:
	.long 0 # argv magic
	.long 0 # command line
	.long 0 # command line length
	.long 0 # argc
	.long 0 # argv
	.long 0 # end address of argv

# header, to be filled in by build script.
offsGOT:   .int 0 # offset of GOT (-4 for lwzu)
sizeGOT:   .int 0 # size of GOT in words
offsEntry: .int 0 # entry point of ELF
sizeBoot:  .int 0 # size of entire injected section

_start2:
    # new entry point for game.
    # we need to recreate everything up to 0x800031A0
    # since it's clobbered by RevolutionOS

    # reproduce __init_registers here
    lis     r1,  0x803F
    ori     r1,  r1,  0x8478
    lis     r2,  0x803E
    ori     r2,  r2,  0x6500
    lis     r13, 0x803E
    ori     r13, r13, 0x31E0
    CALL    0x80003354 # __init_hardware

    li      r0,  -1
    stwu    r1,  -8(r1)
    stw     r0,   4(r1)
    stw     r0,   0(r1)
    CALL    0x80003294 # __init_data

    # let's sneak in here to hook DVDInit
    # and set the arena.
    lis     r3,  0x8024
    ori     r3,  r3,  0x08BC
    bl      .init_getpc
    .init_getpc: mflr r14
    subi    r14, r14, .init_getpc - _start
    addi    r4,  r14, DVDInit_hook - _start
    li      r5,  1 # make bl
    bl      makeBranch

    lwz     r5,  (sizeBoot - _start)(r14)
    add     r5,  r5,  r14
    lis     r4,  0x8000
    stw     r5,  0x0030(r4)

    # now back to our regularly scheduled program.
    li      r0,  0x0
    lis     r6,  0x8000
    addi    r6,  r6,  0x0044 # osDebugExcMask
    stw     r0,  0x0(r6)
    lis     r6,  0x8000
    addi    r6,  r6,  0x00F4 # pDVDBI2
    lwz     r6,  0(r6)
    cmplwi  r6,  0
    beq     .LAB_80003188
    lwz     r7,  0xc(r6)
    b       .LAB_800031a8

.LAB_80003188:
    lis     r5,  0x8000
    addi    r5,  r5,  0x0034 # osArenaHi
    lwz     r5,  0(r5)
    cmplwi  r5,  0
    beq     .LAB_800031d0
    lis     r7,  0x8000
    addi    r7,  r7,  0x30E8 # default_dvd_bi2
    lwz     r7,  0(r7)

# at this point the remaining init code is intact
# so we can just jump back to it.
.LAB_800031a8:
    JUMP    0x800031A8, r0

.LAB_800031d0:
    JUMP    0x800031D0, r0


makeBranch:
    # r3: branch addr
    # r4: dest addr
    # r5: 1 for bl, 0 for b
    # writes branch opcode at *r3.
    # does not check for valid range.
    # clobbers: r6, r7
    sub     r6,  r4,  r3
    lis     r7,  0x03FF
    ori     r7,  r7,  0xFFFC
    and     r6,  r6,  r7
    or      r6,  r6,  r5
    oris    r6,  r6,  0x4800
    stw     r6,  0(r3)
    lis     r7,  0
    dcbi    r7,  r3
    icbi    r7,  r3
    blr

# 0x80243fe8 __OSModuleInit start
# 0x802491f4 DVDInit start

#__OSModuleInit_hook:
#    # reproduce __OSModuleInit here.
#    # we probably don't need to, though...
#    lis     r4,  0x8000
#    li      r0,  0x0
#    stw     r0,  0x30CC(r4) # lastModuleHeader
#    stw     r0,  0x30C8(r4) # firstModuleHeader
#    stw     r0,  0x30D0(r4) # moduleStringTable
#    # patch stuff here...
#    blr

DVDInit_hook:
    stwu    r1,  -STACK_SIZE(r1) # get some stack space
    stmw    r3,  SP_GPR_SAVE(r1)
    mflr    r3
    stw     r3,  SP_LR_SAVE(r1)

    # get the address of this code
    bl      .getpc
    .getpc: mflr r14
    subi    r14, r14, .getpc - _start
    lwz     r5,  (sizeBoot - _start)(r14) # and the size

    # say something to show we're alive.
    .if DEBUG
        addi    r3,  r14, s_hello - _start
        mr      r4,  r14  # address
        mr      r5,  r15  # size
        CALL    OSReport

        lwz     r4,  (offsGOT - _start)(r14)
        lwz     r6,  (sizeGOT - _start)(r14)
        add     r5,  r4,  r14 # r15 = address of GOT - 4
        addi    r3,  r14, s_gotOffs - _start
        CALL    OSReport
    .endif

    # get GOT offset and size
    lwz     r15, (offsGOT  - _start)(r14)
    lwz     r16, (sizeGOT  - _start)(r14)
    addi    r17, r14, (FILE_END - _start)
    add     r15, r15, r14 # r15 = address of GOT - 4

.nextGot:
    cmpwi   r16, 0 # any entries left?
    beq     .doneGot
    lwz     r4,  4(r15) # get GOT entry
    # HACK: there are absolute addresses and even code in here.
    # this seems to work well enough to avoid changing those.
    # XXX figure out the correct way to do this.
    # is there anything that tells us which entries here actually
    # need to be relocated?
    cmpwi   r4,  0
    beq     .setGot
    srwi    r8,  r4,  24 # get high byte
    cmpwi   r8,  0
    bne     .setGot
.if DEBUG
    addi    r3,  r14, s_doReloc - _start
    add     r5,  r4,  r17
    CALL    OSReport
    lwz     r4,  4(r15) # get GOT entry
.endif
    add     r4,  r4,  r17 # to absolute address
.setGot:
    stwu    r4,  4(r15)
    subi    r16, r16, 1
    b       .nextGot
.doneGot:

.if DEBUG
    lwz     r4,  (offsEntry - _start)(r14)
    add     r4,  r4,  r14
    addi    r3,  r14, s_aboutToExec - _start
    CALL    OSReport
.endif

    # jump to code
    lwz     r4,  (offsEntry - _start)(r14)
    add     r4,  r4,  r14
    mtctr   r4
    crclr   4*cr1+eq # not sure, but gcc does this...
    bctrl
.if DEBUG
    addi    r3,  r14, s_doneExec - _start
    CALL    OSReport
.endif

.done:
endSub:
    lwz     r3,  SP_LR_SAVE(r1)
    mtlr    r3   # restore LR
    lmw     r3,  SP_GPR_SAVE(r1)
    addi    r1,  r1,  STACK_SIZE # restore stack ptr
    blr

panic: # r5 = msg
    # apparently OSPanic doesn't work right in Dolphin or something...
    #addi    r3,  r14, s_file - _start
    #li      r4,  0 # line
    #JUMP    OSPanic, r0 # never returns
    mr       r3,  r5
    mr       r4,  r6
    CALL     OSReport
    b        .done

s_noMem:      .string "Out of memory"
s_file:       .string "boot.bin" # for OSPanic
.if DEBUG
    s_hello:        .string "hello, cruel world. addr 0x%X size 0x%X"
    s_gotOffs:      .string "GOT Offs=0x%08X (0x%08X) size=0x%04X"
    s_doReloc:      .string "Reloc %08X -> %08X"
    s_aboutToExec:  .string "Executing at 0x%08X"
    s_doneExec:     .string "Exec OK!"
.endif
.align 4
FILE_END:
# end of bootstrap code. NOT end of all code;
# C code gets appended here.
# this label is used to know where relocation starts.
