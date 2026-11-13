/* GUIA DE LECTURA NXK - kernel/linker.ld
 * Distribuye codigo, datos y variables del kernel en memoria y verifica limites de tamano.
 * Contexto y orden de estudio: ver LEER-CODIGO.md en la carpeta principal.
 */
ENTRY(kernel_entry)
SECTIONS {
    . = 0x10000;
    .text : { *(.text.entry) *(.text*) }
    .rodata : { *(.rodata*) }
    . = ALIGN(4096);
    __sololectura_fin = .;
    .data : { *(.data*) }
    /* El cargador x86 lee hasta 120 KiB; BSS no ocupa bytes en disco.
     * Reserva kernel10000..40000:192KiB incluyendo variables. BSS no altera
     * las lecturas BIOS. La siguiente region fija es la pila en90000;
     * heap100000, NXE180000, tablas200000. Primeros8MiB no se asignan. */
    ASSERT(. <= 0x2e000, "Imagen NXK supera los 120 KiB del cargador x86")
    .bss (NOLOAD) : { __bss_start = .; *(.bss*) *(COMMON) __bss_end = .; }
    ASSERT(. < 0x40000, "Datos NXK superan la reserva de RAM de 192 KiB")
    /DISCARD/ : { *(.eh_frame*) *(.comment*) *(.note*) }
}
