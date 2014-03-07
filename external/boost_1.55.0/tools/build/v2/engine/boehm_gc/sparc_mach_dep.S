!	SPARCompiler 3.0 and later apparently no longer handles
!	asm outside functions.  So we need a separate .s file
!	This is only set up for SunOS 5, not SunOS 4.
!	Assumes this is called before the stack contents are
!	examined.

	.seg 	"text"
	.globl	GC_save_regs_in_stack
	.globl 	GC_push_regs
GC_save_regs_in_stack:
#if defined(__arch64__) || defined(__sparcv9)
	save	%sp,-128,%sp
	flushw
	ret
	  restore %sp,2047+128,%o0
#else /* 32 bit SPARC */
	ta	0x3   ! ST_FLUSH_WINDOWS
	mov	%sp,%o0
	retl
	nop
#endif /* 32 bit SPARC */
.GC_save_regs_in_stack_end:
	.size GC_save_regs_in_stack,.GC_save_regs_in_stack_end-GC_save_regs_in_stack
	

! GC_clear_stack_inner(arg, limit) clears stack area up to limit and
! returns arg.  Stack clearing is crucial on SPARC, so we supply
! an assembly version that s more careful.  Assumes limit is hotter
! than sp, and limit is 8 byte aligned.	
	.globl	GC_clear_stack_inner
GC_clear_stack_inner:
#if defined(__arch64__) || defined(__sparcv9)
	mov %sp,%o2		! Save sp
	add %sp,2047-8,%o3	! p = sp+bias-8
	add %o1,-2047-192,%sp	! Move sp out of the way,
  				! so that traps still work.
  				! Includes some extra words
  				! so we can be sloppy below.
loop:
	stx %g0,[%o3]		! *(long *)p = 0
	cmp %o3,%o1
	bgu,pt %xcc, loop	! if (p > limit) goto loop
          add %o3,-8,%o3	! p -= 8 (delay slot)
	retl
    	  mov %o2,%sp		! Restore sp., delay slot	
#else  /* 32 bit SPARC */
	mov	%sp,%o2		! Save sp
	add	%sp,-8,%o3	! p = sp-8
	clr	%g1		! [g0,g1] = 0
	add	%o1,-0x60,%sp	! Move sp out of the way,
				! so that traps still work.
				! Includes some extra words
				! so we can be sloppy below.
loop:
	std	%g0,[%o3]	! *(long long *)p = 0
	cmp	%o3,%o1
	bgu	loop		! if (p > limit) goto loop
	  add	%o3,-8,%o3	! p -= 8 (delay slot)
	retl
	  mov	%o2,%sp		! Restore sp., delay slot
#endif  /* 32 bit SPARC */
.GC_clear_stack_inner_end:
      	.size GC_clear_stack_inner,.GC_clear_stack_inner_end-GC_clear_stack_inner

	
		
		
		
		
	
