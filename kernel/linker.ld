/* GUIA DE LECTURA NXK - kernel/linker.ld
 * Distribuye codigo, datos y variables del kernel en memoria y verifica limites de tamano.
 * Contexto y orden de estudio: ver LEER-CODIGO.md en la carpeta principal.
 */
ENTRY(kernel_entry)
SECTIONS {
    . = 0x10000;
    .text : { *(.text.entry) *(.text*) }
    .rodata : { *(.rodata*) }
    .data : { *(.data*) }
    .bss (NOLOAD) : { __bss_start = .; *(.bss*) *(COMMON) __bss_end = .; }
    ASSERT(. < 0x18000, "NXK supera el limite inicial de 32 KiB")
    /DISCARD/ : { *(.eh_frame*) *(.comment*) *(.note*) }
}
