	.arch ev6

        .text
        .align  4
        .globl  GC_push_regs
        .ent    GC_push_regs 2
GC_push_regs:
	ldgp    $gp, 0($27)
	lda     $sp, -16($sp)
	stq     $26, 0($sp)
        .mask   0x04000000, 0
        .frame  $sp, 16, $26, 0

/* $0		integer result                                                */
/* $1-$8	temp regs - not preserved cross calls                         */
/* $9-$15	call saved regs                                               */
/* $16-$21	argument regs - not preserved cross calls                     */
/* $22-$28	temp regs - not preserved cross calls                         */
/* $29		global pointer - not preserved cross calls                    */
/* $30		stack pointer                                                 */

# define call_push(x)			\
	mov   x, $16;			\
	jsr   $26, GC_push_one;		\
	ldgp  $gp, 0($26)
	
        call_push($9)
        call_push($10)
        call_push($11)
        call_push($12)
        call_push($13)
        call_push($14)
        call_push($15)

/* $f0-$f1	floating point results                                        */
/* $f2-$f9	call saved regs                                               */
/* $f10-$f30	temp regs - not preserved cross calls                         */

	/* Use the most efficient transfer method for this hardware. */
	/* Bit 1 detects the FIX extension, which includes ftoit. */
	amask	2, $0
	bne	$0, $use_stack

#undef call_push
#define call_push(x)			\
	ftoit	x, $16;			\
	jsr	$26, GC_push_one;	\
	ldgp	$gp, 0($26)

	call_push($f2)
	call_push($f3)
	call_push($f4)
	call_push($f5)
	call_push($f6)
	call_push($f7)
	call_push($f8)
	call_push($f9)

	ldq     $26, 0($sp)
	lda     $sp, 16($sp)
	ret     $31, ($26), 1

	.align	4
$use_stack:

#undef call_push
#define call_push(x)			\
	stt	x, 8($sp);		\
	ldq	$16, 8($sp);		\
	jsr	$26, GC_push_one;	\
	ldgp	$gp, 0($26)

	call_push($f2)
	call_push($f3)
	call_push($f4)
	call_push($f5)
	call_push($f6)
	call_push($f7)
	call_push($f8)
	call_push($f9)

	ldq     $26, 0($sp)
	lda     $sp, 16($sp)
	ret     $31, ($26), 1

	.end    GC_push_regs
