/* Imagen de prueba usuario: una pagina de codigo/constantes, sin datos globales. */
ENTRY(usuario_entrada)
SECTIONS {
 . = 0x400000;
 .text : { *(.text.entry) *(.text*) }
 .rodata : { *(.rodata*) }
 ASSERT(. <= 0x401000, "Aplicacion usuario supera una pagina")
 .data : { *(.data*) }
 .bss (NOLOAD) : { *(.bss*) *(COMMON) }
 ASSERT(SIZEOF(.data) == 0 && SIZEOF(.bss) == 0, "Usuario aun no admite datos globales")
 /DISCARD/ : { *(.eh_frame*) *(.comment*) *(.note*) }
}
