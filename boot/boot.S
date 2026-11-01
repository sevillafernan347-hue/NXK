# GUIA DE LECTURA NXK - boot/boot.S
# Arranque BIOS de la variante de 64 bits: prepara la CPU y transfiere el control al kernel.
# Contexto y orden de estudio: ver LEER-CODIGO.md en la carpeta principal.
/* NXK: cargador BIOS original. Disco virtual, CPU x86-64. */
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
    .quad 0x00af9a000000ffff
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
    mov esp, 0x90000
    /* Tablas de paginas: identidad para los primeros 2 MiB. */
    mov edi, 0x1000
    xor eax, eax
    mov ecx, 3072
    rep stosd
    mov dword ptr [0x1000], 0x2003
    mov dword ptr [0x2000], 0x3003
    mov dword ptr [0x3000], 0x83
    mov eax, cr4
    or eax, 0x20
    mov cr4, eax
    mov eax, 0x1000
    mov cr3, eax
    mov ecx, 0xc0000080
    rdmsr
    or eax, 0x100
    wrmsr
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax
    .byte 0xea
    .long long_mode
    .word 0x18
.code64
long_mode:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    xor eax, eax
    mov fs, ax
    mov gs, ax
    mov rsp, 0x90000
    mov eax, 0x10000
    jmp rax
.org 510
.word 0xaa55
