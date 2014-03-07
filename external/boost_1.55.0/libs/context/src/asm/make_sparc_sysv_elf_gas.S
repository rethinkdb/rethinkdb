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

#define	OFF(N)	(4*(N))
#define	CCFSZ	96
#define	FC_SZ	176
#define	FC_stK	168		// offsetof(fcontext_t, fc_stack)
#define	FC_FPU	0		// offsetof(fcontext_t, fc_fp)
#define	FC_FSR	128		// offsetof(fcontext_t, fc_fp.fp_fsr)
#define	FC_GREG	136		// offsetof(fcontext_t, fc_greg)
#define	BLOCK_SIZE 8

.text
.globl make_fcontext
.align 4
.type make_fcontext,@function
// fcontext_t *
// make_fcontext( void * sp, std::size_t size, void (* fn)( intptr_t) )
make_fcontext:
    save %sp, -CCFSZ, %sp
    // %i0 initial stack pointer
    // %i1 stack size limit
    // %i2 function pointer for context start function

    sub %i0, FC_SZ, %i4			// allocate fcontext_t at on the new stack and keep pointer as return value
    andn %i4, BLOCK_SIZE-1, %i5		// force block ops usable alignement and keep pointer to fcontext in %i5

    st %i0, [%i5+FC_stK+OFF(0)]		// save fs_stack.sp
    st %i1, [%i5+FC_stK+OFF(1)]		// save fs_stack.size
    sub %i5, CCFSZ, %o1   		// leave space for one register window
    st %o1, [%i5+FC_GREG+OFF(0)]	// save new stack pointer
    st %i2, [%i5+FC_GREG+OFF(1)]	// save new %pc (function pointer)
    st %g1, [%i5+FC_GREG+OFF(3)]
    st %g2, [%i5+FC_GREG+OFF(4)]
    st %g3, [%i5+FC_GREG+OFF(5)]
    st %g6, [%i5+FC_GREG+OFF(6)]
    st %g7, [%i5+FC_GREG+OFF(7)]

    // synthesize "return address": jump to finish
    mov %i7, %l0
2:  call 3f
     nop
3:  add finish-2b-8, %o7, %i4
    st %i4, [%i5+FC_GREG+OFF(2)]

    ret
    restore %g0, %i5, %o0	// return fcontext_t

finish:
    mov %g0, %o0
    call _exit
     nop

.size make_fcontext,.-make_fcontext

/* Mark that we don't need executable stack.  */
.section .note.GNU-stack,"",%progbits
