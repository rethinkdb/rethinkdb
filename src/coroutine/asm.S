/* Save current context and install the given one.
   Copyright (C) 2002, 2005 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Andreas Jaeger <aj@suse.de>, 2002.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */


/* int __swapcontext (ucontext_t *oucp, const ucontext_t *ucp);

  Saves the machine context in oucp such that when it is activated,
  it appears as if __swapcontextt() returned again, restores the
  machine context in ucp and thereby resumes execution in that
  context.

  This implementation is intended to be used for *synchronous* context
  switches only.  Therefore, it does not have to save anything
  other than the PRESERVED state.  */

#include "ucontext_i.h"

.globl lightweight_swapcontext
lightweight_swapcontext:
	/* Save the preserved registers, the registers used for passing args,
	   and the return address.  */
	movq	%rbx, oRBX(%rdi)
	movq	%rbp, oRBP(%rdi)
	movq	%r12, oR12(%rdi)
	movq	%r13, oR13(%rdi)
	movq	%r14, oR14(%rdi)
	movq	%r15, oR15(%rdi)

	/* Don't bother saving  and restoring argument registers */
/*	movq	%rdi, oRDI(%rdi)
	movq	%rsi, oRSI(%rdi)
	movq	%rdx, oRDX(%rdi)
	movq	%rcx, oRCX(%rdi)
	movq	%r8, oR8(%rdi)
	movq	%r9, oR9(%rdi) */

	movq	(%rsp), %rcx
	movq	%rcx, oRIP(%rdi)
	leaq	8(%rsp), %rcx		/* Exclude the return address.  */
	movq	%rcx, oRSP(%rdi)


	/* We have separate floating-point register content memory on the
	   stack.  We use the __fpregs_mem block in the context.  Set the
	   links up correctly.  */
/*	leaq	oFPREGSMEM(%rdi), %rcx
	movq	%rcx, oFPREGS(%rdi) */
	/* Save the floating-point environment.  */
/*	fnstenv	(%rcx)
	stmxcsr oMXCSR(%rdi) */


	/* Formerly here: a call to sigprocmask.
	   Deleted because unnecessary for our application. */

	/* Restore the floating-point context.  Not the registers, only the
	   rest.  */
/*	movq	oFPREGS(%rsi), %rcx
	fldenv	(%rcx)
	ldmxcsr oMXCSR(%rsi) */

	/* Load the new stack pointer and the preserved registers.  */
	movq	oRSP(%rsi), %rsp
	movq	oRBX(%rsi), %rbx
	movq	oRBP(%rsi), %rbp
	movq	oR12(%rsi), %r12
	movq	oR13(%rsi), %r13
	movq	oR14(%rsi), %r14
	movq	oR15(%rsi), %r15

	/* The following ret should return to the address set with
	getcontext.  Therefore push the address on the stack.  */
	movq	oRIP(%rsi), %rcx
	pushq	%rcx

	/* Setup registers used for passing args--don't bother with this  */
	movq	oRDI(%rsi), %rdi
/*	movq	oRDX(%rsi), %rdx
	movq	oRCX(%rsi), %rcx
	movq	oR8(%rsi), %r8
	movq	oR9(%rsi), %r9 */

	/* Setup finally  %rsi.  */
	movq	oRSI(%rsi), %rsi

	/* Clear rax to indicate success.  */
	xorl	%eax, %eax
	ret
