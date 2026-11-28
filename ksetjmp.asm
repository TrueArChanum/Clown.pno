; ksetjmp.asm

; Part of Penetrizzel
; Copyright (c) 2026 ArChanum

BITS 32

global ksetjmp
global klongjmp


section .text


; ------------------------------------------------------------
; int setjmp(k_jmp_buf env)
;
; Returns:
;   0 normally
;
; After longjmp:
;   value passed to longjmp()
;
; cdecl:
;   [esp+4] = env
; ------------------------------------------------------------

ksetjmp:

    mov eax,[esp+4]


    ; save callee saved registers

    mov [eax+0],  ebx
    mov [eax+4],  esi
    mov [eax+8],  edi
    mov [eax+12], ebp


    ; save stack pointer  BUG!
    ;mov [eax+16], esp

    ; Save the stack pointer as it will be after setjmp returns.
    lea ecx, [esp+4]
    mov [eax+16], ecx


    ; save return address
    mov ecx,[esp]
    mov [eax+20], ecx


    ; save flags

    pushfd
    pop ecx

    mov [eax+24], ecx


    ; normal return

    xor eax,eax

    ret



; ------------------------------------------------------------
; void longjmp(k_jmp_buf env, int value)
;
; Does not return.
;
; cdecl:
;   [esp+4] = env
;   [esp+8] = value
; ------------------------------------------------------------

klongjmp:

    mov edx,[esp+4]


    ; return value

    mov eax,[esp+8]

    test eax,eax
    jne .value_ok

    mov eax,1


.value_ok:


    ; restore registers

    mov ebx,[edx+0]
    mov esi,[edx+4]
    mov edi,[edx+8]
    mov ebp,[edx+12]


    ; restore flags

    push dword [edx+24]
    popfd


    ; restore stack

    mov esp,[edx+16]


    ; jump back into setjmp()

    jmp [edx+20]

;END
