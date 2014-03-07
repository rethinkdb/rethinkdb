
;           Copyright Oliver Kowalke 2009.
;  Distributed under the Boost Software License, Version 1.0.
;     (See accompanying file LICENSE_1_0.txt or copy at
;           http://www.boost.org/LICENSE_1_0.txt)

;  --------------------------------------------------------------
;  |    0    |    1    |    2    |    3    |    4     |    5    |
;  --------------------------------------------------------------
;  |    0h   |   04h   |   08h   |   0ch   |   010h   |   014h  |
;  --------------------------------------------------------------
;  |   EDI   |   ESI   |   EBX   |   EBP   |   ESP    |   EIP   |
;  --------------------------------------------------------------
;  --------------------------------------------------------------
;  |    6    |    7    |    8    |                              |
;  --------------------------------------------------------------
;  |   018h  |   01ch  |   020h  |                              |
;  --------------------------------------------------------------
;  |    sp   |   size  |  limit  |                              |
;  --------------------------------------------------------------
;  --------------------------------------------------------------
;  |    9    |                                                  |
;  --------------------------------------------------------------
;  |  024h   |                                                  |
;  --------------------------------------------------------------
;  |fc_execpt|                                                  |
;  --------------------------------------------------------------
;  --------------------------------------------------------------
;  |   10    |                                                  |
;  --------------------------------------------------------------
;  |  028h   |                                                  |
;  --------------------------------------------------------------
;  |fc_strage|                                                  |
;  --------------------------------------------------------------
;  --------------------------------------------------------------
;  |   11    |    12   |                                        |
;  --------------------------------------------------------------
;  |  02ch   |   030h  |                                        |
;  --------------------------------------------------------------
;  | fc_mxcsr|fc_x87_cw|                                        |
;  --------------------------------------------------------------
;  --------------------------------------------------------------
;  |   13    |                                                  |
;  --------------------------------------------------------------
;  |  034h   |                                                  |
;  --------------------------------------------------------------
;  |fc_deallo|                                                  |
;  --------------------------------------------------------------

.386
.XMM
.model flat, c
.code

jump_fcontext PROC EXPORT
    mov     ecx,         [esp+04h]   ; load address of the first fcontext_t arg
    mov     [ecx],       edi         ; save EDI
    mov     [ecx+04h],   esi         ; save ESI
    mov     [ecx+08h],   ebx         ; save EBX
    mov     [ecx+0ch],   ebp         ; save EBP

    assume  fs:nothing
    mov     edx,         fs:[018h]   ; load NT_TIB
    assume  fs:error
    mov     eax,         [edx]       ; load current SEH exception list
    mov     [ecx+024h],  eax         ; save current exception list
    mov     eax,         [edx+04h]   ; load current stack base
    mov     [ecx+018h],  eax         ; save current stack base
    mov     eax,         [edx+08h]   ; load current stack limit
    mov     [ecx+020h],  eax         ; save current stack limit
    mov     eax,         [edx+0e0ch] ; load current deallocation stack
    mov     [ecx+034h],  eax         ; save current deallocation stack
    mov     eax,         [edx+010h]  ; load fiber local storage
    mov     [ecx+028h],  eax         ; save fiber local storage

    lea     eax,         [esp+04h]   ; exclude the return address
    mov     [ecx+010h],  eax         ; save as stack pointer
    mov     eax,         [esp]       ; load return address
    mov     [ecx+014h],  eax         ; save return address

    mov     edx,        [esp+08h]    ; load address of the second fcontext_t arg
    mov     edi,        [edx]        ; restore EDI
    mov     esi,        [edx+04h]    ; restore ESI
    mov     ebx,        [edx+08h]    ; restore EBX
    mov     ebp,        [edx+0ch]    ; restore EBP

    mov     eax,        [esp+010h]   ; check if fpu enve preserving was requested
    test    eax,        eax 
    je      nxt

    stmxcsr [ecx+02ch]               ; save MMX control word
    fnstcw  [ecx+030h]               ; save x87 control word
    ldmxcsr [edx+02ch]               ; restore MMX control word
    fldcw   [edx+030h]               ; restore x87 control word
nxt:
    mov     ecx,         edx
    assume  fs:nothing
    mov     edx,         fs:[018h]   ; load NT_TIB
    assume  fs:error
    mov     eax,         [ecx+024h]  ; load SEH exception list
    mov     [edx],       eax         ; restore next SEH item
    mov     eax,         [ecx+018h]  ; load stack base
    mov     [edx+04h],   eax         ; restore stack base
    mov     eax,         [ecx+020h]  ; load stack limit
    mov     [edx+08h],   eax         ; restore stack limit
    mov     eax,         [ecx+034h]  ; load deallocation stack
    mov     [edx+0e0ch], eax         ; restore deallocation stack
    mov     eax,         [ecx+028h]  ; load fiber local storage
    mov     [edx+010h],  eax         ; restore fiber local storage

    mov     eax,         [esp+0ch]   ; use third arg as return value after jump

    mov     esp,         [ecx+010h]  ; restore ESP
    mov     [esp+04h],   eax         ; use third arg as first arg in context function
    mov     ecx,         [ecx+014h]  ; fetch the address to return to

    jmp     ecx                     ; indirect jump to context
jump_fcontext ENDP
END
