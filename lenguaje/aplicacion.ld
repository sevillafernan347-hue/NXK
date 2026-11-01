/* GUIA DE LECTURA NXK - lenguaje/aplicacion.ld
 * Reglas de enlace de apps NXE: ubicacion fija, punto de entrada y limite de memoria.
 * Contexto y orden de estudio: ver LEER-CODIGO.md en la carpeta principal.
 */
ENTRY(nxe_entrada)
SECTIONS {
    . = 0x180000;
    .text : { *(.text.entry) *(.text*) }
    .rodata : { *(.rodata*) }
    .data : { *(.data*) }
    .bss (NOLOAD) : { *(.bss*) *(COMMON) }
    ASSERT(. <= 0x1a0000, "La aplicacion NXE supera 128 KiB")
    /DISCARD/ : { *(.eh_frame*) *(.comment*) *(.note*) }
}
