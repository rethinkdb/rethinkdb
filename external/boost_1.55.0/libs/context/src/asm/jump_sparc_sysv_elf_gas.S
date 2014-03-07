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

#define OFF(N)  (4*(N))
#define CCFSZ   96
#define FC_SZ   176
#define FC_stK  168     // offsetof(fcontext_t, fc_stack)
#define FC_FPU  0       // offsetof(fcontext_t, fc_fp)
#define FC_FSR  128     // offsetof(fcontext_t, fc_fp.fp_fsr)
#define FC_GREG 136     // offsetof(fcontext_t, fc_greg)
#define BLOCK_SIZE 8
#ifdef __NetBSD__
#define FLUSHW  t 0x83; nop // T_FLUSHWIN
#endif

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

    FLUSHW // make sure all shadow registers are up to date in the current stack

    // save current state to fcontext_t at %o0
    st %sp, [%o0 + FC_GREG + OFF(0)]    // current stack pointer
    add %o7, 8, %o4         // calculate next instruction past call
    st %o4, [%o0 + FC_GREG + OFF(1)]    // and store it as %pc in save context
    st %o7, [%o0 + FC_GREG + OFF(2)]
    st %g1, [%o0 + FC_GREG + OFF(3)]
    st %g2, [%o0 + FC_GREG + OFF(4)]
    st %g3, [%o0 + FC_GREG + OFF(5)]
    st %g6, [%o0 + FC_GREG + OFF(6)]
    st %g7, [%o0 + FC_GREG + OFF(7)]

    // do we need to handle fpu?
    cmp %o3, 0
    bz Lno_fpu
    nop

    add %o0, FC_FPU, %o5
    std %f0, [%o5]
    std %f2, [%o5+0x08]
    std %f4, [%o5+0x10]
    std %f6, [%o5+0x18]
    std %f8, [%o5+0x20]
    std %f10, [%o5+0x28]
    std %f12, [%o5+0x30]
    std %f14, [%o5+0x38]
    st %fsr, [%o0+FC_FSR]

    add %o1, FC_FPU, %o5
    ldd [%o5], %f0
    ldd [%o5+0x08], %f2
    ldd [%o5+0x10], %f4
    ldd [%o5+0x18], %f6
    ldd [%o5+0x20], %f8
    ldd [%o5+0x28], %f10
    ldd [%o5+0x30], %f12
    ldd [%o5+0x38], %f14
    ld [%o1+FC_FSR], %fsr

Lno_fpu:
    // load new state from %o1
    ld [%o1 + FC_GREG + OFF(1)], %o4
    ld [%o1 + FC_GREG + OFF(2)], %o7
    ld [%o1 + FC_GREG + OFF(3)], %g1
    ld [%o1 + FC_GREG + OFF(4)], %g2
    ld [%o1 + FC_GREG + OFF(5)], %g3
    ld [%o1 + FC_GREG + OFF(6)], %g6
    ld [%o1 + FC_GREG + OFF(7)], %g7
    // switch to new stack
    ld [%o1 + FC_GREG + OFF(0)], %sp
    // and now reload from this stack the shadow regist bank contents
    ld [%sp + OFF(0)], %l0
    ld [%sp + OFF(1)], %l1
    ld [%sp + OFF(2)], %l2
    ld [%sp + OFF(3)], %l3
    ld [%sp + OFF(4)], %l4
    ld [%sp + OFF(5)], %l5
    ld [%sp + OFF(6)], %l6
    ld [%sp + OFF(7)], %l7
    ld [%sp + OFF(8)], %i0
    ld [%sp + OFF(9)], %i1
    ld [%sp + OFF(10)], %i2
    ld [%sp + OFF(11)], %i3
    ld [%sp + OFF(12)], %i4
    ld [%sp + OFF(13)], %i5
    ld [%sp + OFF(14)], %i6
    ld [%sp + OFF(15)], %i7

    // finally continue execution in new context
    jmp %o4
    mov %o2, %o0    // return arg as result

.size jump_fcontext,.-jump_fcontext

/* Mark that we don't need executable stack.  */
.section .note.GNU-stack,"",%progbits
