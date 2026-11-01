/* GUIA DE LECTURA NXK - lib/nxk.h
 * Declaraciones compartidas: reserva de memoria, operaciones de bytes y comparacion de texto.
 * Contexto y orden de estudio: ver LEER-CODIGO.md en la carpeta principal.
 */
#ifndef NXK_H
#define NXK_H
#include <stddef.h>
#include <stdint.h>
void *nxk_alloc(size_t bytes);
size_t nxk_heap_used(void);
size_t nxk_heap_available(void);
uint32_t nxk_ram_kib(void);
void *memset(void *dest, int value, size_t bytes);
/* Las regiones no deben solaparse. */
void *memcpy(void *dest, const void *source, size_t bytes);
void *memmove(void *dest, const void *source, size_t bytes);
int nxk_equal(const char *a, const char *b);
#endif
