#define	setcontext(u)	setmcontext(&(u)->uc_mcontext)
#define	getcontext(u)	getmcontext(&(u)->uc_mcontext)
typedef struct mcontext mcontext_t;
typedef struct ucontext ucontext_t;


extern	int		swapcontext(ucontext_t*, const ucontext_t*);
extern	void		makecontext(ucontext_t*, void(*)(void), int, ...);
extern	int		getmcontext(mcontext_t*);
extern	void		setmcontext(const mcontext_t*);

/*-
 * Copyright (c) 1999 Marcel Moolenaar
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer 
 *    in this position and unchanged.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/sys/ucontext.h,v 1.4 1999/10/11 20:33:17 luoqi Exp $
 */

/* #include <machine/ucontext.h> */

/*-
 * Copyright (c) 1999 Marcel Moolenaar
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer 
 *    in this position and unchanged.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/i386/include/ucontext.h,v 1.4 1999/10/11 20:33:09 luoqi Exp $
 */

struct mcontext {
	/*
	 * The first 20 fields must match the definition of
	 * sigcontext. So that we can support sigcontext
	 * and ucontext_t at the same time.
	 */
	long	mc_onstack;		/* XXX - sigcontext compat. */
	long	mc_rdi;			/* machine state (struct trapframe) */
	long	mc_rsi;
	long	mc_rdx;
	long	mc_rcx;
	long	mc_r8;
	long	mc_r9;
	long	mc_rax;
	long	mc_rbx;
	long	mc_rbp;
	long	mc_r10;
	long	mc_r11;
	long	mc_r12;
	long	mc_r13;
	long	mc_r14;
	long	mc_r15;
	long	mc_trapno;
	long	mc_addr;
	long	mc_flags;
	long	mc_err;
	long	mc_rip;
	long	mc_cs;
	long	mc_rflags;
	long	mc_rsp;
	long	mc_ss;

	long	mc_len;			/* sizeof(mcontext_t) */
#define	_MC_FPFMT_NODEV		0x10000	/* device not present or configured */
#define	_MC_FPFMT_XMM		0x10002
	long	mc_fpformat;
#define	_MC_FPOWNED_NONE	0x20000	/* FP state not used */
#define	_MC_FPOWNED_FPU		0x20001	/* FP state came from FPU */
#define	_MC_FPOWNED_PCB		0x20002	/* FP state came from PCB */
	long	mc_ownedfp;
	/*
	 * See <machine/fpu.h> for the internals of mc_fpstate[].
	 */
	long	mc_fpstate[64];
	long	mc_spare[8];
};

struct ucontext {
	/*
	 * Keep the order of the first two fields. Also,
	 * keep them the first two fields in the structure.
	 * This way we can have a union with struct
	 * sigcontext and ucontext_t. This allows us to
	 * support them both at the same time.
	 * note: the union is not defined, though.
	 */
	sigset_t	uc_sigmask;
	mcontext_t	uc_mcontext;

	struct __ucontext *uc_link;
	stack_t		uc_stack;
	int		__spare__[8];
};


