!	SPARCompiler 3.0 and later apparently no longer handles
!	asm outside functions.  So we need a separate .s file
!	This is only set up for SunOS 4.
!	Assumes this is called before the stack contents are
!	examined.

#include "machine/asm.h"

	.seg 	"text"
	.globl	_C_LABEL(GC_save_regs_in_stack)
	.globl 	_C_LABEL(GC_push_regs)
_C_LABEL(GC_save_regs_in_stack):
_C_LABEL(GC_push_regs):
	ta	0x3   ! ST_FLUSH_WINDOWS
	mov	%sp,%o0
	retl
	nop
	
	.globl	_C_LABEL(GC_clear_stack_inner)
_C_LABEL(GC_clear_stack_inner):
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
