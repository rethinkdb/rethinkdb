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
 *  |    sp   |   size  |                                        |  *
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
.globl _jump_fcontext
.align 2
_jump_fcontext:
    movl    0x4(%esp), %ecx         /* load address of the first fcontext_t arg */
    movl    %edi,       (%ecx)      /* save EDI */
    movl    %esi,       0x4(%ecx)   /* save ESI */
    movl    %ebx,       0x8(%ecx)   /* save EBX */
    movl    %ebp,       0xc(%ecx)   /* save EBP */

    leal    0x4(%esp),  %eax        /* exclude the return address */
    movl    %eax,       0x10(%ecx)  /* save as stack pointer */
    movl    (%esp),     %eax        /* load return address */
    movl    %eax,       0x14(%ecx)  /* save return address */

    movl    0x8(%esp),   %edx       /* load address of the second fcontext_t arg */
    movl    (%edx),      %edi       /* restore EDI */
    movl    0x4(%edx),   %esi       /* restore ESI */
    movl    0x8(%edx),   %ebx       /* restore EBX */
    movl    0xc(%edx),   %ebp       /* restore EBP */

    movl    0x10(%esp),  %eax       /* check if fpu enve preserving was requested */
    test    %eax,        %eax
    je      1f

    stmxcsr  0x20(%ecx)             /* save MMX control and status word */
    fnstcw   0x24(%ecx)             /* save x87 control word */
    ldmxcsr  0x20(%edx)             /* restore MMX control and status word */
    fldcw    0x24(%edx)             /* restore x87 control word */
1:
    movl    0xc(%esp),   %eax       /* use third arg as return value after jump */

    movl    0x10(%edx),  %esp       /* restore ESP */
    movl    %eax,        0x4(%esp)  /* use third arg as first arg in context function */
    movl    0x14(%edx),  %edx       /* fetch the address to return to */

    jmp     *%edx                   /* indirect jump to context */
