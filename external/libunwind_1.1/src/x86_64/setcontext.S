/* libunwind - a platform-independent unwind library
   Copyright (C) 2007 Google, Inc
	Contributed by Arun Sharma <arun.sharma@google.com>
   Copyright (C) 2010 Konstantin Belousov <kib@freebsd.org>

This file is part of libunwind.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.  */

#include "ucontext_i.h"
#if defined __linux__
#include <asm/unistd.h>
#define	SIG_SETMASK   2
#define	SIGSET_BYTE_SIZE   (64/8)
#elif defined __FreeBSD__
#include <sys/syscall.h>
#endif

/*  int _Ux86_64_setcontext (const ucontext_t *ucp)

  Restores the machine context provided.
  Unlike the libc implementation, doesn't clobber %rax
  
*/
	.global _Ux86_64_setcontext
	.type _Ux86_64_setcontext, @function

_Ux86_64_setcontext:

#if defined __linux__
	/* restore signal mask
           sigprocmask(SIG_SETMASK, ucp->uc_sigmask, NULL, sizeof(sigset_t)) */
	push %rdi
	mov $__NR_rt_sigprocmask, %rax
	lea UC_SIGMASK(%rdi), %rsi
	mov $SIG_SETMASK, %rdi
	xor %rdx, %rdx
	mov $SIGSET_BYTE_SIZE, %r10
	syscall
	pop %rdi

        /* restore fp state */
	mov    UC_MCONTEXT_FPREGS_PTR(%rdi),%r8
	fldenv (%r8)
	ldmxcsr FPREGS_OFFSET_MXCSR(%r8)
#elif defined __FreeBSD__
	/* restore signal mask */
	pushq	%rdi
	xorl	%edx,%edx
	leaq	UC_SIGMASK(%rdi),%rsi
	movl	$3,%edi/* SIG_SETMASK */
	movl	$SYS_sigprocmask,%eax
	movq	%rcx,%r10
	syscall
	popq	%rdi

	/* restore fp state */
	cmpq $UC_MCONTEXT_FPOWNED_FPU,UC_MCONTEXT_OWNEDFP(%rdi)
	jne 1f
	cmpq $UC_MCONTEXT_FPFMT_XMM,UC_MCONTEXT_FPFORMAT(%rdi)
	jne 1f
	fxrstor UC_MCONTEXT_FPSTATE(%rdi)
1:
#else
#error Port me
#endif

	/* restore the rest of the state */
	mov    UC_MCONTEXT_GREGS_R8(%rdi),%r8
	mov    UC_MCONTEXT_GREGS_R9(%rdi),%r9
	mov    UC_MCONTEXT_GREGS_RBX(%rdi),%rbx
	mov    UC_MCONTEXT_GREGS_RBP(%rdi),%rbp
	mov    UC_MCONTEXT_GREGS_R12(%rdi),%r12
	mov    UC_MCONTEXT_GREGS_R13(%rdi),%r13
	mov    UC_MCONTEXT_GREGS_R14(%rdi),%r14
	mov    UC_MCONTEXT_GREGS_R15(%rdi),%r15
	mov    UC_MCONTEXT_GREGS_RSI(%rdi),%rsi
	mov    UC_MCONTEXT_GREGS_RDX(%rdi),%rdx
	mov    UC_MCONTEXT_GREGS_RAX(%rdi),%rax
	mov    UC_MCONTEXT_GREGS_RCX(%rdi),%rcx
	mov    UC_MCONTEXT_GREGS_RSP(%rdi),%rsp

        /* push the return address on the stack */
	mov    UC_MCONTEXT_GREGS_RIP(%rdi),%rcx
	push   %rcx

	mov    UC_MCONTEXT_GREGS_RCX(%rdi),%rcx
	mov    UC_MCONTEXT_GREGS_RDI(%rdi),%rdi
	retq

	.size _Ux86_64_setcontext, . - _Ux86_64_setcontext

      /* We do not need executable stack.  */
      .section        .note.GNU-stack,"",@progbits
