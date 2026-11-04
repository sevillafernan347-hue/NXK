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
     * Reservamos hasta 128 KiB totales de RAM para kernel y variables. */
    ASSERT(. <= 0x2e000, "Imagen NXK supera los 120 KiB del cargador x86")
    .bss (NOLOAD) : { __bss_start = .; *(.bss*) *(COMMON) __bss_end = .; }
    ASSERT(. < 0x30000, "Datos NXK superan la reserva de RAM de 128 KiB")
    /DISCARD/ : { *(.eh_frame*) *(.comment*) *(.note*) }
}
