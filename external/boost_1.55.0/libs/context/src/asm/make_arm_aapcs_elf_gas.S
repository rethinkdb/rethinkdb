/*
            Copyright Oliver Kowalke 2009.
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENSE_1_0.txt or copy at
          http://www.boost.org/LICENSE_1_0.txt)
*/

/*******************************************************************
 *                                                                 *
 *  -------------------------------------------------------------  *
 *  |  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |  *
 *  -------------------------------------------------------------  *
 *  | 0x0 | 0x4 | 0x8 | 0xc | 0x10| 0x14| 0x18| 0x1c| 0x20| 0x24|  *
 *  -------------------------------------------------------------  *
 *  |  v1 |  v2 |  v3 |  v4 |  v5 |  v6 |  v7 |  v8 |  sp |  lr |  *
 *  -------------------------------------------------------------  *
 *  -------------------------------------------------------------  *
 *  |  10 |                                                     |  *
 *  -------------------------------------------------------------  *
 *  | 0x28|                                                     |  *
 *  -------------------------------------------------------------  *
 *  |  pc |                                                     |  *
 *  -------------------------------------------------------------  *
 *  -------------------------------------------------------------  *
 *  |  11 |  12 |                                               |  *
 *  -------------------------------------------------------------  *
 *  | 0x2c| 0x30|                                               |  *
 *  -------------------------------------------------------------  *
 *  |  sp | size|                                               |  *
 *  -------------------------------------------------------------  *
 *  -------------------------------------------------------------  *
 *  |  13 |  14 | 15 |  16 |  17 |  18 |  19 |  20 |  21 |  22  |  *
 *  -------------------------------------------------------------  *
 *  | 0x34| 0x38|0x3c| 0x40| 0x44| 0x48| 0x4c| 0x50| 0x54| 0x58 |  *
 *  -------------------------------------------------------------  *
 *  | s16 | s17 | s18 | s19 | s20 | s21 | s22 | s23 | s24 | s25 |  *
 *  -------------------------------------------------------------  *
 *  -------------------------------------------------------------  *
 *  |  23 |  24 |  25 |  26 |  27 |  28 |                       |  *
 *  -------------------------------------------------------------  *
 *  | 0x5c| 0x60| 0x64| 0x68| 0x6c| 0x70|                       |  *
 *  -------------------------------------------------------------  *
 *  | s26 | s27 | s28 | s29 | s30 | s31 |                       |  *
 *  -------------------------------------------------------------  *
 *                                                                 *
 * *****************************************************************/

.text
.globl make_fcontext
.align 2
.type make_fcontext,%function
make_fcontext:
    mov     a4, a1          @ save address of context stack (base) A4
    sub     a1, a1, #116    @ reserve space for fcontext_t at top of context stack

    @ shift address in A1 to lower 16 byte boundary
    @ == pointer to fcontext_t and address of context stack
    bic     a1, a1, #15

    str     a4, [a1,#44]    @ save address of context stack (base) in fcontext_t
    str     a2, [a1,#48]    @ save context stack size in fcontext_t
    str     a3, [a1,#40]    @ save address of context function in fcontext_t

    str     a1, [a1,#32]    @ save address in A4 as stack pointer for context function

    adr     a2, finish      @ compute abs address of label finish
    str     a2, [a1,#36]    @ save address of finish as return address for context function
                            @ entered after context function returns

    bx      lr

finish:
    @ SP points to same addras SP on entry of context function
    mov     a1, #0          @ exit code is zero
    bl      _exit@PLT       @ exit application
.size make_fcontext,.-make_fcontext

/* Mark that we don't need executable stack.  */
.section .note.GNU-stack,"",%progbits
