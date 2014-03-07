# define call_push(x)     move    $4,x;    jal     GC_push_one

    .text
 # Mark from machine registers that are saved by C compiler
    .globl  GC_push_regs
    .ent    GC_push_regs
GC_push_regs:
    subu    $sp,8       ## Need to save only return address
    sw      $31,4($sp)
    .mask   0x80000000,-4
    .frame  $sp,8,$31
    call_push($2)
    call_push($3)
    call_push($16)
    call_push($17)
    call_push($18)
    call_push($19)
    call_push($20)
    call_push($21)
    call_push($22)
    call_push($23)
    call_push($30)
    lw      $31,4($sp)
    addu    $sp,8
    j       $31
    .end    GC_push_regs
