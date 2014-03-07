/*
            Copyright Oliver Kowalke 2009.
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENSE_1_0.txt or copy at
            http://www.boost.org/LICENSE_1_0.txt)
*/

/****************************************************************************************
 *                                                                                      *
 *  ----------------------------------------------------------------------------------  *
 *  |    0    |    1    |    2    |    3    |    4     |    5    |    6    |    7    |  *
 *  ----------------------------------------------------------------------------------  *
 *  |   0x0   |   0x4   |   0x8   |   0xc   |   0x10   |   0x14  |   0x18  |   0x1c  |  *
 *  ----------------------------------------------------------------------------------  *
 *  |        RBX        |        R12        |         R13        |        R14        |  *
 *  ----------------------------------------------------------------------------------  *
 *  ----------------------------------------------------------------------------------  *
 *  |    8    |    9    |   10    |   11    |    12    |    13   |    14   |    15   |  *
 *  ----------------------------------------------------------------------------------  *
 *  |   0x20  |   0x24  |   0x28  |  0x2c   |   0x30   |   0x34  |   0x38  |   0x3c  |  *
 *  ----------------------------------------------------------------------------------  *
 *  |        R15        |        RBP        |         RSP        |        RIP        |  *
 *  ----------------------------------------------------------------------------------  *
 *  ----------------------------------------------------------------------------------  *
 *  |   16    |   17    |   18    |    19   |                                        |  *
 *  ----------------------------------------------------------------------------------  *
 *  |  0x40   |  0x44   |  0x48   |   0x4c  |                                        |  *
 *  ----------------------------------------------------------------------------------  *
 *  |        sp         |       size        |                                        |  *
 *  ----------------------------------------------------------------------------------  *
 *  ----------------------------------------------------------------------------------  *
 *  |    20   |    21   |                                                            |  *
 *  ----------------------------------------------------------------------------------  *
 *  |   0x50  |   0x54  |                                                            |  *
 *  ----------------------------------------------------------------------------------  *
 *  | fc_mxcsr|fc_x87_cw|                                                            |  *
 *  ----------------------------------------------------------------------------------  *
 *                                                                                      *
 * **************************************************************************************/

.text
.globl make_fcontext
.type make_fcontext,@function
.align 16
make_fcontext:
    leaq   -0x58(%rdi),    %rax        /* reserve space for fcontext_t at top of context stack */

    /* shift address in RAX to lower 16 byte boundary */
    /* == pointer to fcontext_t and address of context stack */
    andq   $-16,           %rax

    movq   %rdi,           0x40(%rax) /* save address of context stack pointer (base) in fcontext_t */
    movq   %rsi,           0x48(%rax) /* save context stack size in fcontext_t */
    movq   %rdx,           0x38(%rax) /* save address of context function in fcontext_t */

    stmxcsr  0x50(%rax)                /* save MMX control and status word */
    fnstcw   0x54(%rax)                /* save x87 control word */

    leaq   -0x8(%rax),      %rdx       /* reserve space for the return address on context stack, (RSP - 0x8) % 16 == 0 */
    movq   %rdx,            0x30(%rax) /* save address in RDX as stack pointer for context function */

    leaq   finish(%rip),    %rcx       /* compute abs address of label finish */
    movq   %rcx,            (%rdx)     /* save address of finish as return address for context function */
                                       /* entered after context function returns */

    ret                                /* return pointer to fcontext_t placed on context stack */

finish:
    /* RSP points to same address as RSP on entry of context function + 0x8 */
    xorq    %rdi,           %rdi       /* exit code is zero */
    call   _exit@PLT                   /* exit application */
    hlt
.size make_fcontext,.-make_fcontext

/* Mark that we don't need executable stack.  */
.section .note.GNU-stack,"",%progbits
