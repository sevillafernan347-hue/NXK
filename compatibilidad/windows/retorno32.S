# GUIA DE LECTURA NXK - compatibilidad/windows/retorno32.S
# Guarda la pila del kernel al entrar al EXE y la recupera cuando este termina.
# Contexto y orden de estudio: ver LEER-CODIGO.md en la carpeta principal.
.intel_syntax noprefix
.section .text
.code32
.global nxk_pe_salto, nxk_pe_regresar
nxk_pe_salto:
    push ebp
    push ebx
    push esi
    push edi
    pushfd
    mov dword ptr [nxk_pe_pila], esp
    mov eax, dword ptr [esp+24]
    and esp, -16
    call eax
nxk_pe_regresar:
    mov esp, dword ptr [nxk_pe_pila]
    popfd
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret
.section .note.GNU-stack,"",@progbits
