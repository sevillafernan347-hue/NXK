# GUIA DE LECTURA NXK - kernel/entry32.S
# Referencia antigua de la entrada de 32 bits, sustituida por entrada32.nxa.
# Contexto y orden de estudio: ver LEER-CODIGO.md en la carpeta principal.
.intel_syntax noprefix
.section .text.entry
.code32
.global kernel_entry
kernel_entry:
    cld
    mov edi, offset __bss_start
    mov ecx, offset __bss_end
    sub ecx, edi
    xor eax, eax
    rep stosb
    call kernel_main
1:  cli
    hlt
    jmp 1b
.section .note.GNU-stack,"",@progbits
