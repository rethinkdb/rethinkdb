/*
            Copyright Oliver Kowalke 2009.
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENSE_1_0.txt or copy at
          http://www.boost.org/LICENSE_1_0.txt)
*/

/********************************************************************
 *                                                                  *
 *  --------------------------------------------------------------  *
 *  |    0    |    1    |    2    |    3    |    4     |    5    |  *
 *  --------------------------------------------------------------  *
 *  |   0x0   |   0x4   |   0x8   |   0xc   |   0x10   |   0x14  |  *
 *  --------------------------------------------------------------  *
 *  |   EDI   |   ESI   |   EBX   |   EBP   |   ESP    |   EIP   |  *
 *  --------------------------------------------------------------  *
 *  --------------------------------------------------------------  *
 *  |    6    |    7    |                                        |  *
 *  --------------------------------------------------------------  *
 *  |   0x18  |   0x1c  |                                        |  *
 *  --------------------------------------------------------------  *
 *  |    sp   |  size   |                                        |  *
 *  --------------------------------------------------------------  *
 *  --------------------------------------------------------------  *
 *  |    8    |    9    |                                        |  *
 *  --------------------------------------------------------------  *
 *  |   0x20  |   0x24  |                                        |  *
 *  --------------------------------------------------------------  *
 *  | fc_mxcsr|fc_x87_cw|                                        |  *
 *  --------------------------------------------------------------  *
 *                                                                  *
 * *****************************************************************/

.text
.globl make_fcontext
.align 2
.type make_fcontext,@function
make_fcontext:
    movl   0x4(%esp),      %eax         /* load 1. arg of make_fcontext, pointer to context stack (base) */
    leal   -0x28(%eax),    %eax         /* reserve space for fcontext_t at top of context stack */

    /* shift address in EAX to lower 16 byte boundary */
    /* == pointer to fcontext_t and address of context stack */
    andl   $-16,           %eax

    movl   0x4(%esp),      %edx         /* load 1. arg of make_fcontext, pointer to context stack (base) */
    movl   %edx,           0x18(%eax)   /* save address of context stack (base) in fcontext_t */
    movl   0x8(%esp),      %edx         /* load 2. arg of make_fcontext, context stack size */
    movl   %edx,           0x1c(%eax)   /* save stack size in fcontext_t */
    movl   0xc(%esp),      %edx         /* load 3. arg of make_fcontext, pointer to context function */
    movl   %edx,           0x14(%eax)   /* save address of context function in fcontext_t */

    stmxcsr  0x20(%eax)                 /* save MMX control and status word */
    fnstcw   0x24(%eax)                 /* save x87 control word */

    leal   -0x14(%eax),    %edx         /* reserve space for the last frame on context stack; (ESP - 0x4) % 16 == 0 */
    movl   %edx,           0x10(%eax)   /* save address in EDX as stack pointer for context function */

    call   1f
1:  popl   %ecx                        /* address of label 2 */
    addl   $finish-1b, %ecx            /* compute abs address of label finish */
    movl   %ecx, (%edx)                /* save address of finish as return address for context functions */
                                       /* entered after context function returns */

    ret

finish:
    /* ESP points to same address as ESP on entry of context function + 0x4 */
    call    2f
2:  popl    %ebx                                    /* address of label 3 */
    addl    $_GLOBAL_OFFSET_TABLE_+[.-2b], %ebx     /* compute address of GOT and store it in EBX */

    xorl    %eax,  %eax
    movl    %eax,  (%esp)               /* exit code is zero */
    call   _exit@PLT                    /* exit application */
    hlt
.size make_fcontext,.-make_fcontext

/* Mark that we don't need executable stack.  */
.section .note.GNU-stack,"",%progbits
