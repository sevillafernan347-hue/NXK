/* GUIA DE LECTURA NXK - boot/boot.ld
 * Reglas de enlace del sector de arranque de referencia: ubican el codigo para el BIOS.
 * Contexto y orden de estudio: ver LEER-CODIGO.md en la carpeta principal.
 */
ENTRY(_start)
SECTIONS { . = 0x7c00; .text : { *(.text) } /DISCARD/ : { *(.eh_frame*) *(.comment*) } }
