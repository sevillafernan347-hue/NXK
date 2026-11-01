/* Puente temporal GNU asm: entrada usuario y salida sincronica al llamador.
 * Pendiente expresarlo en NEXTRY ASM. Solo una ejecucion activa por CPU. */
.code32
.section .text
.global nxk_usuario_entrar, nxk_usuario_salir, nxk_usuario_abortar
nxk_usuario_entrar:
    pushfl
    pushl %ebp
    pushl %ebx
    pushl %esi
    pushl %edi
    movl %esp, nxk_usuario_pila_guardada
    movl 24(%esp), %ecx
    movl 28(%esp), %edx
    cli
    movw $51, %ax
    movw %ax, %ds
    movw %ax, %es
    pushl $51
    pushl %edx
    pushl $2
    pushl $43
    pushl %ecx
    iret
nxk_usuario_salir:
    /* La puerta de interrupcion cambia SS:ESP usando la TSS. */
    cld
    movl 4(%esp), %ecx
    movl %ecx, nxk_usuario_cs_guardado
    jmp restaurar_kernel
nxk_usuario_abortar:
    movl 4(%esp), %eax
restaurar_kernel:
    pushl %eax
    movw $16, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    popl %eax
    movl nxk_usuario_pila_guardada, %esp
    popl %edi
    popl %esi
    popl %ebx
    popl %ebp
    popfl
    ret
.section .note.GNU-stack,"",@progbits
