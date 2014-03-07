#include <sys/regdef.h>
#include <sys/asm.h>
/* This file must be preprocessed.  But the SGI assembler always does	*/
/* that.  Furthermore, a generic preprocessor won't do, since some of	*/
/* the SGI-supplied include files rely on behavior of the MIPS 		*/
/* assembler.  Hence we treat and name this file as though it required	*/
/* no preprocessing.							*/

# define call_push(x)     move    $4,x;    jal     GC_push_one

    .option pic2
    .text
/* Mark from machine registers that are saved by C compiler */
#   define FRAMESZ 32
#   define RAOFF FRAMESZ-SZREG
#   define GPOFF FRAMESZ-(2*SZREG)
    NESTED(GC_push_regs, FRAMESZ, ra)
    .mask 0x80000000,-SZREG	# inform debugger of saved ra loc
    move 	t0,gp
    SETUP_GPX(t8)
    PTR_SUBU	sp,FRAMESZ
#   ifdef SETUP_GP64
      SETUP_GP64(GPOFF, GC_push_regs)
#   endif
    SAVE_GP(GPOFF)
    REG_S 	ra,RAOFF(sp)
#   if (_MIPS_SIM == _MIPS_SIM_ABI32)
    	call_push($2)
    	call_push($3)
#   endif
    call_push($16)
    call_push($17)
    call_push($18)
    call_push($19)
    call_push($20)
    call_push($21)
    call_push($22)
    call_push($23)
    call_push($30)
    REG_L	ra,RAOFF(sp)
#   ifdef RESTORE_GP64
      RESTORE_GP64
#   endif
    PTR_ADDU	sp,FRAMESZ
    j		ra
    .end    GC_push_regs
