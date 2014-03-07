        .text
        .align 16
        .global GC_save_regs_in_stack
        .proc GC_save_regs_in_stack
GC_save_regs_in_stack:
        .body
        flushrs
        ;;
        mov r8=ar.bsp
        br.ret.sptk.few rp
        .endp GC_save_regs_in_stack

