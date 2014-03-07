
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
_exit PROTO, value:SDWORD
.code

make_fcontext PROC EXPORT
    mov  eax,         [esp+04h]     ; load 1. arg of make_fcontext, pointer to context stack (base)
    lea  eax,         [eax-038h]    ; reserve space for fcontext_t at top of context stack

    ; shift address in EAX to lower 16 byte boundary
    ; == pointer to fcontext_t and address of context stack
    and  eax,         -16

    mov  ecx,         [esp+04h]     ; load 1. arg of make_fcontext, pointer to context stack (base)
    mov  [eax+018h],  ecx           ; save address of context stack (base) in fcontext_t
    mov  edx,         [esp+08h]     ; load 2. arg of make_fcontext, context stack size
    mov  [eax+01ch],  edx           ; save context stack size in fcontext_t
    neg  edx                        ; negate stack size for LEA instruction (== substraction)
    lea  ecx,         [ecx+edx]     ; compute bottom address of context stack (limit)
    mov  [eax+020h],  ecx           ; save address of context stack (limit) in fcontext_t
    mov  [eax+034h],  ecx           ; save address of context stack limit as 'dealloction stack'
    mov  ecx,         [esp+0ch]     ; load 3. arg of make_fcontext, pointer to context function
    mov  [eax+014h],  ecx           ; save address of context function in fcontext_t

    stmxcsr [eax+02ch]              ; save MMX control word
    fnstcw  [eax+030h]              ; save x87 control word

    lea  edx,         [eax-024h]    ; reserve space for last frame and seh on context stack, (ESP - 0x4) % 16 == 0
    mov  [eax+010h],  edx           ; save address in EDX as stack pointer for context function

    mov  ecx,         finish        ; abs address of finish
    mov  [edx],       ecx           ; save address of finish as return address for context function
                                    ; entered after context function returns

    ; traverse current seh chain to get the last exception handler installed by Windows
    ; note that on Windows Server 2008 and 2008 R2, SEHOP is activated by default
    ; the exception handler chain is tested for the presence of ntdll.dll!FinalExceptionHandler
    ; at its end by RaiseException all seh andlers are disregarded if not present and the
    ; program is aborted
    assume  fs:nothing
    mov     ecx,      fs:[018h]     ; load NT_TIB into ECX
    assume  fs:error

walk:
    mov  edx,         [ecx]         ; load 'next' member of current SEH into EDX
    inc  edx                        ; test if 'next' of current SEH is last (== 0xffffffff)
    jz   found
    dec  edx
    xchg edx,         ecx           ; exchange content; ECX contains address of next SEH
    jmp  walk                       ; inspect next SEH

found:
    mov  ecx,         [ecx+04h]     ; load 'handler' member of SEH == address of last SEH handler installed by Windows
    mov  edx,         [eax+010h]    ; load address of stack pointer for context function
    mov  [edx+018h],  ecx           ; save address in ECX as SEH handler for context
    mov  ecx,         0ffffffffh    ; set ECX to -1
    mov  [edx+014h],  ecx           ; save ECX as next SEH item
    lea  ecx,         [edx+014h]    ; load address of next SEH item
    mov  [eax+024h],  ecx           ; save next SEH

    ret

finish:
    ; ESP points to same address as ESP on entry of context function + 0x4
    xor   eax,        eax
    mov   [esp],      eax           ; exit code is zero
    call  _exit                     ; exit application
    hlt
make_fcontext ENDP
END
