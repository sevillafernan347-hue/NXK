# GUIA DE LECTURA NXK - kernel/excepciones32.S
# Referencia anterior del manejador de excepciones, sustituida por excepciones32.nxa.
# Contexto y orden de estudio: ver LEER-CODIGO.md en la carpeta principal.
.intel_syntax noprefix
.section .text
.code32
.global nxk_excepcion_entrada
nxk_excepcion_entrada:
    cli
    cld
    pushad
    and esp, -16
    call nxk_excepcion_fatal
1:  hlt
    jmp 1b
.section .note.GNU-stack,"",@progbits
