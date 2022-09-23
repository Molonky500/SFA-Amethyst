# set register names
.set r0,0;   .set r1,1;   .set r2,2; .set r3,3;   .set r4,4
.set r5,5;   .set r6,6;   .set r7,7;   .set r8,8;   .set r9,9
.set r10,10; .set r11,11; .set r12,12; .set r13,13; .set r14,14
.set r15,15; .set r16,16; .set r17,17; .set r18,18; .set r19,19
.set r20,20; .set r21,21; .set r22,22; .set r23,23; .set r24,24
.set r25,25; .set r26,26; .set r27,27; .set r28,28; .set r29,29
.set r30,30; .set r31,31; .set f0,0; .set f2,2; .set f3,3
.set f0,0;   .set f1,1;   .set f2,2; .set f3,3;   .set f4,4
.set f5,5;   .set f6,6;   .set f7,7;   .set f8,8;   .set f9,9
.set f10,10; .set f11,11; .set f12,12; .set f13,13; .set f14,14
.set f15,15; .set f16,16; .set f17,17; .set f18,18; .set f19,19
.set f20,20; .set f21,21; .set f22,22; .set f23,23; .set f24,24
.set f25,25; .set f26,26; .set f27,27; .set f28,28; .set f29,29
.set f30,30; .set f31,31; .set f0,0; .set f2,2; .set f3,3
.set SPRG0,272
.set XER,1
.set SRR0,26
.set SRR1,27
.set MSR_RI,  0x00000002
.set MSR_DR,  0x00000010
.set MSR_IR,  0x00000020
.set MSR_IP,  0x00000040
.set MSR_SE,  0x00000400
.set MSR_ME,  0x00001000
.set MSR_FP,  0x00002000
.set MSR_POW, 0x00004000
.set MSR_EE,  0x00008000
.set IBAT0U, 528
.set IBAT0L, 529
.set IBAT1U, 530
.set IBAT1L, 531
.set IBAT2U, 532
.set IBAT2L, 533
.set IBAT3U, 534
.set IBAT3L, 535
.set IBAT4U, 560
.set IBAT4L, 561
.set IBAT5U, 562
.set IBAT5L, 563
.set IBAT6U, 564
.set IBAT6L, 565
.set IBAT7U, 566
.set IBAT7L, 567
.set DBAT0U, 536
.set DBAT0L, 537
.set DBAT1U, 538
.set DBAT1L, 539
.set DBAT2U, 540
.set DBAT2L, 541
.set DBAT3U, 542
.set DBAT3L, 543
.set DBAT4U, 568
.set DBAT4L, 569
.set DBAT5U, 570
.set DBAT5L, 571
.set DBAT6U, 572
.set DBAT6L, 573
.set DBAT7U, 574
.set DBAT7L, 575
.set EXCEPTION_FRAME_END,728

.extern c_exception_handler
.global exceptionHook
#.global _raw_exceptionHook_Reset
.global _raw_exceptionHook_MachineCheck
.global _raw_exceptionHook_DSI
.global _raw_exceptionHook_ISI
.global _raw_exceptionHook_External
.global _raw_exceptionHook_Alignment
.global _raw_exceptionHook_Program
.global _raw_exceptionHook_FpUnavailable
.global _raw_exceptionHook_Decrementer
.global _raw_exceptionHook_Syscall
.global _raw_exceptionHook_Trace
.global _raw_exceptionHook_PerfMon
.global _raw_exceptionHook_IABR
#.global _raw_exceptionHook_Thermal
.global _raw_exceptionHook_External2
.global _raw_exceptionHook_External_END
.global _raw_exceptionHook_FpUnavailable2
.global _raw_exceptionHook_FpUnavailable_END
.global get_r1
.global get_r2
.global get_r13
.global set_r1
.global set_r2
.global set_r13
.global get_msr
.global set_msr

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

# has to come first because lol
.org 0x00001600
_raw_exceptionHook_FpUnavailable:
    nop        # replaced by our patcher
    mtspr      SPRG0, r3
    lis        r3,  0x0000

    # save CTR and use it to jump because lol
    # note we still use 0x1700 here
    mfctr      r2
    stw        r2,  (.ctr_save - _raw_exceptionHook_External + 0x1700)(r3)
    lis        r2,  0x0000
    ori        r2,  r2,  0x0804 # not 0x0504
    mtctr      r2

    # grab the libogc values for these regs
    lwz        r2,  (.ogc_r2 - _raw_exceptionHook_External + 0x1700)(r3)
    lwz        r13, (.ogc_r13 - _raw_exceptionHook_External + 0x1700)(r3)
    mfspr      r3, SPRG0

    # jump back to the default handler
    bctr
    # we can reuse the end of the other hook

    # this isn't working because we "rfi" into the libogc
    # thread handling code so it gets the wrong r13.
    # so a modified libogc might be required...

_raw_exceptionHook_FpUnavailable_END:

.org 0x00001700
_raw_exceptionHook_External:
    nop        # replaced by our patcher
    mtspr      SPRG0, r3
    lis        r3,  0x0000

    # save CTR and use it to jump because lol
    # stupid math here because lol
    mfctr      r2
    stw        r2,  (.ctr_save - _raw_exceptionHook_External + 0x1700)(r3)
    lis        r2,  0x0000
    ori        r2,  r2,  0x0504
    mtctr      r2

    # grab the libogc values for these regs
    lwz        r2,  (.ogc_r2 - _raw_exceptionHook_External + 0x1700)(r3)
    lwz        r13, (.ogc_r13 - _raw_exceptionHook_External + 0x1700)(r3)
    mfspr      r3, SPRG0

    # jump back to the default handler
    bctr

# we also patch the default handler to jump back here
# instead of rfi
_raw_exceptionHook_External2:
    # restore CTR
    lis        r2,  0x0000
    lwz        r13, (.ctr_save - _raw_exceptionHook_External + 0x1700)(r2)
    mtctr      r13

    # restore game's regs (these never change)
    lis        r2,  0x803E
    ori        r2,  r2,  0x6500
    lis        r13, 0x803E
    ori        r13, r13, 0x31E0
    rfi

.ctr_save:
    .long 0xDEADBEEF
.ogc_r2:
    .long 0xDEADBEEF
.ogc_r13:
    .long 0xDEADBEEF
_raw_exceptionHook_External_END:


constants:
    .set LR_SAVE,    0x00
    .set XER_SAVE,   0x04
    .set SRR0_SAVE,  0x08
    .set SRR1_SAVE,  0x0C
    .set GPR_SAVE,   0x10

.macro EXCEPTION_COMMON code
    mtspr      SPRG0, r3
    lis        r3,  0x0000
    ori        r3,  r3, 0x0100
    stmw       r0,  GPR_SAVE(r3)
    mr         r12, r3
    mfspr      r3,  SPRG0
    stw        r3,  (GPR_SAVE+16)(r12)
    mflr       r3
    stw        r3,  LR_SAVE(r12)

    mfspr      r4,  XER
    stw        r4,  XER_SAVE(r12)
    mfspr      r4,  SRR0
    stw        r4,  SRR0_SAVE(r12)
    mfspr      r4,  SRR1
    stw        r4,  SRR1_SAVE(r12)
    dcbf       r0,  r12
    mr         r3,  r12
    lis        r12, 0x8000
    or         r3,  r3,  r12
    lis        r4,  \code@h
    ori        r4,  r4,  \code@l
    lis        r12, c_exception_handler@h
    addi       r12, r12,c_exception_handler@l
    mtspr      SRR0,r12
    rfi
.endm

#_raw_exceptionHook_Reset:
#    EXCEPTION_COMMON 1

_raw_exceptionHook_MachineCheck:
    EXCEPTION_COMMON 2

_raw_exceptionHook_DSI:
    EXCEPTION_COMMON 3

_raw_exceptionHook_ISI:
    EXCEPTION_COMMON 4

# External is below

_raw_exceptionHook_Alignment:
    EXCEPTION_COMMON 6

_raw_exceptionHook_Program:
    EXCEPTION_COMMON 7

#_raw_exceptionHook_FpUnavailable:
#    EXCEPTION_COMMON 8

_raw_exceptionHook_Decrementer:
    EXCEPTION_COMMON 9

_raw_exceptionHook_Syscall:
    EXCEPTION_COMMON 0xC

_raw_exceptionHook_Trace:
    EXCEPTION_COMMON 0xD

# 0xE doesn't exist

_raw_exceptionHook_PerfMon:
    EXCEPTION_COMMON 0xF

# more nonexistent items

_raw_exceptionHook_IABR:
    EXCEPTION_COMMON 0x13

# and more

#_raw_exceptionHook_Thermal:
#    EXCEPTION_COMMON 0x17
# thermal interrupt isn't hooked up on Wii
# so we'll just stash some globals in its place
