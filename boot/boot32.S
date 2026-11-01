# GUIA DE LECTURA NXK - boot/boot32.S
# Referencia antigua del arranque de 32 bits. La construccion actual usa arranque32.nxa.
# Contexto y orden de estudio: ver LEER-CODIGO.md en la carpeta principal.
/* NXK: cargador BIOS original para x86 de 32 bits. */
.intel_syntax noprefix
.section .text
.code16
.global _start
_start:
    .byte 0xea
    .word start, 0
start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00
    cld
    sti
    mov si, offset packet
    mov ah, 0x42
    int 0x13
    jc disk_error
    cli
    in al, 0x92
    or al, 2
    out 0x92, al
    lgdt [gdt_pointer]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    .byte 0xea
    .word protected, 0x08
disk_error:
    mov ax, 0x0e45
    int 0x10
    cli
1:  hlt
    jmp 1b
.balign 8
packet:
    .byte 16, 0
    .word 64
    .word 0, 0x1000
    .quad 1
gdt:
    .quad 0
    .quad 0x00cf9a000000ffff
    .quad 0x00cf92000000ffff
gdt_end:
gdt_pointer:
    .word gdt_end-gdt-1
    .long gdt
.code32
protected:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax
    mov esp, 0x90000
    /* No requiere modo largo, PAE ni paginacion. */
    mov eax, 0x10000
    jmp eax
.org 510
.word 0xaa55
