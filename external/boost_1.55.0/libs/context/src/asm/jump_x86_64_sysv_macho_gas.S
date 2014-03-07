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
 *  |        sp         |        size       |                                        |  *
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
.globl _jump_fcontext
.align 8
_jump_fcontext:
    movq     %rbx,       (%rdi)         /* save RBX */
    movq     %r12,       0x8(%rdi)      /* save R12 */
    movq     %r13,       0x10(%rdi)     /* save R13 */
    movq     %r14,       0x18(%rdi)     /* save R14 */
    movq     %r15,       0x20(%rdi)     /* save R15 */
    movq     %rbp,       0x28(%rdi)     /* save RBP */

    cmp      $0,         %rcx
    je       1f

    stmxcsr  0x50(%rdi)             /* save MMX control and status word */
    fnstcw   0x54(%rdi)             /* save x87 control word */

    ldmxcsr  0x50(%rsi)             /* restore MMX control and status word */
    fldcw    0x54(%rsi)             /* restore x87 control word */
1:

    leaq     0x8(%rsp),  %rax       /* exclude the return address and save as stack pointer */
    movq     %rax,       0x30(%rdi) /* save as stack pointer */
    movq     (%rsp),     %rax       /* save return address */
    movq     %rax,       0x38(%rdi) /* save return address as RIP */

    movq     (%rsi),      %rbx      /* restore RBX */
    movq     0x8(%rsi),   %r12      /* restore R12 */
    movq     0x10(%rsi),  %r13      /* restore R13 */
    movq     0x18(%rsi),  %r14      /* restore R14 */
    movq     0x20(%rsi),  %r15      /* restore R15 */
    movq     0x28(%rsi),  %rbp      /* restore RBP */

    movq     0x30(%rsi),  %rsp      /* restore RSP */
    movq     0x38(%rsi),  %rcx      /* fetch the address to return to */

    movq     %rdx,        %rax      /* use third arg as return value after jump */
    movq     %rdx,        %rdi      /* use third arg as first arg in context function */

    jmp      *%rcx                  /* indirect jump to context */
