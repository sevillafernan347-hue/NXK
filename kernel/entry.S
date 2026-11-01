# GUIA DE LECTURA NXK - kernel/entry.S
# Entrada de 64 bits: prepara el entorno inicial y llama al kernel en C.
# Contexto y orden de estudio: ver LEER-CODIGO.md en la carpeta principal.
.intel_syntax noprefix
.section .text.entry
.global kernel_entry
kernel_entry:
    cld
    lea rdi, [rip + __bss_start]
    lea rcx, [rip + __bss_end]
    sub rcx, rdi
    xor eax, eax
    rep stosb
    call kernel_main
1:  cli
    hlt
    jmp 1b
.section .note.GNU-stack,"",@progbits
