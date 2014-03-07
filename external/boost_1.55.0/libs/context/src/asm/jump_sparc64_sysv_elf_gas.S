/*
            Copyright Martin Husemann 2013.
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENSE_1_0.txt or copy at
          http://www.boost.org/LICENSE_1_0.txt)
*/

/*******************************************************************
 *                                                                 *
 *  -------------------------------------------------------------  *
 *  |  Offset (in 4 or 8 byte units) | Content                  |  *
 *  -------------------------------------------------------------  *
 *  | 0                              | %sp                      |  *
 *  -------------------------------------------------------------  *
 *  | 1                              | %pc                      |  *
 *  -------------------------------------------------------------  *
 *  | 2                              | %i7 (return address)     |  *
 *  -------------------------------------------------------------  *
 *  | 3                              | %g1                      |  *
 *  -------------------------------------------------------------  *
 *  | 4                              | %g2                      |  *
 *  -------------------------------------------------------------  *
 *  | 5                              | %g3                      |  *
 *  -------------------------------------------------------------  *
 *  | 6                              | %g6                      |  *
 *  -------------------------------------------------------------  *
 *  | 7                              | %g7                      |  *
 *  -------------------------------------------------------------  *
 *    The local and in registers are stored on the stack.          *
 *******************************************************************/

#define OFF(N)	(8*(N))
#define CCFSZ   176		// C Compiler Frame Size
#define BIAS    (2048-1)	// Stack offset for 64 bit programs
#define FC_SZ   448		// sizeof(fcontext_t)
#define FC_STK  384		// offsetof(fcontext_t, fc_stack)
#define FC_FPU  0		// offsetof(fcontext_t, fc_fp)
#define FC_FSR  264		// offsetof(fcontext_t, fc_fp.fp_fsr)
#define FC_FPRS 256		// offsetof(fcontext_t, fc_fp.fp_fprs)
#define FC_GREG 320		// offsetof(fcontext_t, fc_greg)
#define BLOCK_SIZE  64

    .register %g2,#ignore
    .register %g3,#ignore
    .register %g6,#ignore

.text
.globl jump_fcontext
.align 4
.type jump_fcontext,@function
// intptr_t
// jump_fcontext( fcontext_t * ofc, fcontext_t const* nfc, intptr_t vp,
//                bool preserve_fpu = true);
jump_fcontext:
    // %o0 = pointer to old fcontext, save current state here
    // %o1 = new context to jump to
    // %o2 = new return value in context %o0
    // %o3 = preserve fpu registers
    // Save current state in %o0 fcontext, then activate %o1.
    // If %o3, include fpu registers.

    flushw // make sure all shadow registers are up to date in the current stack

    // save current state to fcontext_t at %o0
    stx %sp, [%o0 + FC_GREG + OFF(0)]    // current stack pointer
    add %o7, 8, %o4         // calculate next instruction past call
    stx %o4, [%o0 + FC_GREG + OFF(1)]    // and store it as %pc in save context
    stx %o7, [%o0 + FC_GREG + OFF(2)]
    stx %g1, [%o0 + FC_GREG + OFF(3)]
    stx %g2, [%o0 + FC_GREG + OFF(4)]
    stx %g3, [%o0 + FC_GREG + OFF(5)]
    stx %g6, [%o0 + FC_GREG + OFF(6)]
    stx %g7, [%o0 + FC_GREG + OFF(7)]

    // do we need to handle fpu?
    brz %o3, Lno_fpu
    nop

    add %o0, FC_FPU, %o5
    stda %f0, [%o5] 0xf0 /* ASI_BLOCK_PRIMARY */
    add %o5, BLOCK_SIZE, %o5
    stda %f16, [%o5] 0xf0
    add %o5, BLOCK_SIZE, %o5
    stda %f32, [%o5] 0xf0
    add %o5, BLOCK_SIZE, %o5
    stda %f48, [%o5] 0xf0
    stx %fsr, [%o0+FC_FSR]
    rd %fprs, %o4
    stx %o4, [%o0+FC_FPRS]

    add %o1, FC_FPU, %o5
    ldda [%o5] 0xf0 /* ASI_BLOCK_PRIMARY */, %f0
    add %o5, BLOCK_SIZE, %o5
    ldda [%o5] 0xf0, %f16
    add %o5, BLOCK_SIZE, %o5
    ldda [%o5] 0xf0, %f32
    add %o5, BLOCK_SIZE, %o5
    ldda [%o5] 0xf0, %f48
    ldx [%o1+FC_FSR], %fsr
    ldx [%o1+FC_FPRS], %o4
    wr %o4,0,%fprs

Lno_fpu:
    // load new state from %o1
    ldx [%o1 + FC_GREG + OFF(1)], %o4
    ldx [%o1 + FC_GREG + OFF(2)], %o7
    ldx [%o1 + FC_GREG + OFF(3)], %g1
    ldx [%o1 + FC_GREG + OFF(4)], %g2
    ldx [%o1 + FC_GREG + OFF(5)], %g3
    ldx [%o1 + FC_GREG + OFF(6)], %g6
    ldx [%o1 + FC_GREG + OFF(7)], %g7
    // switch to new stack
    ldx [%o1 + FC_GREG + OFF(0)], %sp
    // and now reload from this stack the shadow regist bank contents
    ldx [%sp + BIAS + OFF(0)], %l0
    ldx [%sp + BIAS + OFF(1)], %l1
    ldx [%sp + BIAS + OFF(2)], %l2
    ldx [%sp + BIAS + OFF(3)], %l3
    ldx [%sp + BIAS + OFF(4)], %l4
    ldx [%sp + BIAS + OFF(5)], %l5
    ldx [%sp + BIAS + OFF(6)], %l6
    ldx [%sp + BIAS + OFF(7)], %l7
    ldx [%sp + BIAS + OFF(8)], %i0
    ldx [%sp + BIAS + OFF(9)], %i1
    ldx [%sp + BIAS + OFF(10)], %i2
    ldx [%sp + BIAS + OFF(11)], %i3
    ldx [%sp + BIAS + OFF(12)], %i4
    ldx [%sp + BIAS + OFF(13)], %i5
    ldx [%sp + BIAS + OFF(14)], %i6
    ldx [%sp + BIAS + OFF(15)], %i7

    // finally continue execution in new context
    jmp %o4
    mov %o2, %o0    // return arg as result

.size jump_fcontext,.-jump_fcontext

/* Mark that we don't need executable stack.  */
.section .note.GNU-stack,"",%progbits
