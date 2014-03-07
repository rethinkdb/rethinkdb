.file	"x86cpuid.s"
.text
.globl	_OPENSSL_ia32_cpuid
.align	4
_OPENSSL_ia32_cpuid:
L_OPENSSL_ia32_cpuid_begin:
	pushl	%ebp
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	xorl	%edx,%edx
	pushfl
	popl	%eax
	movl	%eax,%ecx
	xorl	$2097152,%eax
	pushl	%eax
	popfl
	pushfl
	popl	%eax
	xorl	%eax,%ecx
	xorl	%eax,%eax
	btl	$21,%ecx
	jnc	L000nocpuid
	.byte	0x0f,0xa2
	movl	%eax,%edi
	xorl	%eax,%eax
	cmpl	$1970169159,%ebx
	setne	%al
	movl	%eax,%ebp
	cmpl	$1231384169,%edx
	setne	%al
	orl	%eax,%ebp
	cmpl	$1818588270,%ecx
	setne	%al
	orl	%eax,%ebp
	jz	L001intel
	cmpl	$1752462657,%ebx
	setne	%al
	movl	%eax,%esi
	cmpl	$1769238117,%edx
	setne	%al
	orl	%eax,%esi
	cmpl	$1145913699,%ecx
	setne	%al
	orl	%eax,%esi
	jnz	L001intel
	movl	$2147483648,%eax
	.byte	0x0f,0xa2
	cmpl	$2147483649,%eax
	jb	L001intel
	movl	%eax,%esi
	movl	$2147483649,%eax
	.byte	0x0f,0xa2
	orl	%ecx,%ebp
	andl	$2049,%ebp
	cmpl	$2147483656,%esi
	jb	L001intel
	movl	$2147483656,%eax
	.byte	0x0f,0xa2
	movzbl	%cl,%esi
	incl	%esi
	movl	$1,%eax
	.byte	0x0f,0xa2
	btl	$28,%edx
	jnc	L002generic
	shrl	$16,%ebx
	andl	$255,%ebx
	cmpl	%esi,%ebx
	ja	L002generic
	andl	$4026531839,%edx
	jmp	L002generic
L001intel:
	cmpl	$4,%edi
	movl	$-1,%edi
	jb	L003nocacheinfo
	movl	$4,%eax
	movl	$0,%ecx
	.byte	0x0f,0xa2
	movl	%eax,%edi
	shrl	$14,%edi
	andl	$4095,%edi
L003nocacheinfo:
	movl	$1,%eax
	.byte	0x0f,0xa2
	andl	$3220176895,%edx
	cmpl	$0,%ebp
	jne	L004notintel
	orl	$1073741824,%edx
	andb	$15,%ah
	cmpb	$15,%ah
	jne	L004notintel
	orl	$1048576,%edx
L004notintel:
	btl	$28,%edx
	jnc	L002generic
	andl	$4026531839,%edx
	cmpl	$0,%edi
	je	L002generic
	orl	$268435456,%edx
	shrl	$16,%ebx
	cmpb	$1,%bl
	ja	L002generic
	andl	$4026531839,%edx
L002generic:
	andl	$2048,%ebp
	andl	$4294965247,%ecx
	movl	%edx,%esi
	orl	%ecx,%ebp
	btl	$27,%ecx
	jnc	L005clear_avx
	xorl	%ecx,%ecx
.byte	15,1,208
	andl	$6,%eax
	cmpl	$6,%eax
	je	L006done
	cmpl	$2,%eax
	je	L005clear_avx
L007clear_xmm:
	andl	$4261412861,%ebp
	andl	$4278190079,%esi
L005clear_avx:
	andl	$4026525695,%ebp
L006done:
	movl	%esi,%eax
	movl	%ebp,%edx
L000nocpuid:
	popl	%edi
	popl	%esi
	popl	%ebx
	popl	%ebp
	ret
.globl	_OPENSSL_rdtsc
.align	4
_OPENSSL_rdtsc:
L_OPENSSL_rdtsc_begin:
	xorl	%eax,%eax
	xorl	%edx,%edx
	call	L008PIC_me_up
L008PIC_me_up:
	popl	%ecx
	movl	L_OPENSSL_ia32cap_P$non_lazy_ptr-L008PIC_me_up(%ecx),%ecx
	btl	$4,(%ecx)
	jnc	L009notsc
	.byte	0x0f,0x31
L009notsc:
	ret
.globl	_OPENSSL_instrument_halt
.align	4
_OPENSSL_instrument_halt:
L_OPENSSL_instrument_halt_begin:
	call	L010PIC_me_up
L010PIC_me_up:
	popl	%ecx
	movl	L_OPENSSL_ia32cap_P$non_lazy_ptr-L010PIC_me_up(%ecx),%ecx
	btl	$4,(%ecx)
	jnc	L011nohalt
.long	2421723150
	andl	$3,%eax
	jnz	L011nohalt
	pushfl
	popl	%eax
	btl	$9,%eax
	jnc	L011nohalt
	.byte	0x0f,0x31
	pushl	%edx
	pushl	%eax
	hlt
	.byte	0x0f,0x31
	subl	(%esp),%eax
	sbbl	4(%esp),%edx
	addl	$8,%esp
	ret
L011nohalt:
	xorl	%eax,%eax
	xorl	%edx,%edx
	ret
.globl	_OPENSSL_far_spin
.align	4
_OPENSSL_far_spin:
L_OPENSSL_far_spin_begin:
	pushfl
	popl	%eax
	btl	$9,%eax
	jnc	L012nospin
	movl	4(%esp),%eax
	movl	8(%esp),%ecx
.long	2430111262
	xorl	%eax,%eax
	movl	(%ecx),%edx
	jmp	L013spin
.align	4,0x90
L013spin:
	incl	%eax
	cmpl	(%ecx),%edx
	je	L013spin
.long	529567888
	ret
L012nospin:
	xorl	%eax,%eax
	xorl	%edx,%edx
	ret
.globl	_OPENSSL_wipe_cpu
.align	4
_OPENSSL_wipe_cpu:
L_OPENSSL_wipe_cpu_begin:
	xorl	%eax,%eax
	xorl	%edx,%edx
	call	L014PIC_me_up
L014PIC_me_up:
	popl	%ecx
	movl	L_OPENSSL_ia32cap_P$non_lazy_ptr-L014PIC_me_up(%ecx),%ecx
	movl	(%ecx),%ecx
	btl	$1,(%ecx)
	jnc	L015no_x87
.long	4007259865,4007259865,4007259865,4007259865,2430851995
L015no_x87:
	leal	4(%esp),%eax
	ret
.globl	_OPENSSL_atomic_add
.align	4
_OPENSSL_atomic_add:
L_OPENSSL_atomic_add_begin:
	movl	4(%esp),%edx
	movl	8(%esp),%ecx
	pushl	%ebx
	nop
	movl	(%edx),%eax
L016spin:
	leal	(%eax,%ecx,1),%ebx
	nop
.long	447811568
	jne	L016spin
	movl	%ebx,%eax
	popl	%ebx
	ret
.globl	_OPENSSL_indirect_call
.align	4
_OPENSSL_indirect_call:
L_OPENSSL_indirect_call_begin:
	pushl	%ebp
	movl	%esp,%ebp
	subl	$28,%esp
	movl	12(%ebp),%ecx
	movl	%ecx,(%esp)
	movl	16(%ebp),%edx
	movl	%edx,4(%esp)
	movl	20(%ebp),%eax
	movl	%eax,8(%esp)
	movl	24(%ebp),%eax
	movl	%eax,12(%esp)
	movl	28(%ebp),%eax
	movl	%eax,16(%esp)
	movl	32(%ebp),%eax
	movl	%eax,20(%esp)
	movl	36(%ebp),%eax
	movl	%eax,24(%esp)
	call	*8(%ebp)
	movl	%ebp,%esp
	popl	%ebp
	ret
.globl	_OPENSSL_cleanse
.align	4
_OPENSSL_cleanse:
L_OPENSSL_cleanse_begin:
	movl	4(%esp),%edx
	movl	8(%esp),%ecx
	xorl	%eax,%eax
	cmpl	$7,%ecx
	jae	L017lot
	cmpl	$0,%ecx
	je	L018ret
L019little:
	movb	%al,(%edx)
	subl	$1,%ecx
	leal	1(%edx),%edx
	jnz	L019little
L018ret:
	ret
.align	4,0x90
L017lot:
	testl	$3,%edx
	jz	L020aligned
	movb	%al,(%edx)
	leal	-1(%ecx),%ecx
	leal	1(%edx),%edx
	jmp	L017lot
L020aligned:
	movl	%eax,(%edx)
	leal	-4(%ecx),%ecx
	testl	$-4,%ecx
	leal	4(%edx),%edx
	jnz	L020aligned
	cmpl	$0,%ecx
	jne	L019little
	ret
.globl	_OPENSSL_ia32_rdrand
.align	4
_OPENSSL_ia32_rdrand:
L_OPENSSL_ia32_rdrand_begin:
	movl	$8,%ecx
L021loop:
.byte	15,199,240
	jc	L022break
	loop	L021loop
L022break:
	cmpl	$0,%eax
	cmovel	%ecx,%eax
	ret
.section __IMPORT,__pointers,non_lazy_symbol_pointers
L_OPENSSL_ia32cap_P$non_lazy_ptr:
.indirect_symbol	_OPENSSL_ia32cap_P
.long	0
.comm	_OPENSSL_ia32cap_P,8,2
.mod_init_func
.align 2
.long   _OPENSSL_cpuid_setup
