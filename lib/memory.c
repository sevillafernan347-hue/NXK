/* GUIA DE LECTURA NXK - lib/memory.c
 * Asignador incremental de 512 KiB y comparacion de cadenas. Incluye rutinas de memoria C para x64.
 * Contexto y orden de estudio: ver LEER-CODIGO.md en la carpeta principal.
 */
#include "nxk.h"
/* Area fija de laboratorio; aun no es un gestor de memoria fisica. */
static uintptr_t next = 0x100000;
void *nxk_alloc(size_t bytes) {
    /* Cada reserva avanza el cursor a un multiplo de 16. No existe free todavia.
       Se rechaza lo que no cabe antes de sumar, para evitar desbordamientos. */
    if (!bytes || bytes > 0x80000) return 0;
    size_t aligned = (bytes + 15) & ~(size_t)15;
    if (aligned > 0x180000 - next) return 0;
    void *result = (void *)next;
    next += aligned;
    memset(result, 0, aligned);
    return result;
}
size_t nxk_heap_used(void) { return next - 0x100000; }
/* Espacio restante del heap de laboratorio, no toda la RAM del equipo. */
size_t nxk_heap_available(void) { return 0x180000 - next; }
#if !defined(__i386__)
void *memmove(void *dest, const void *source, size_t bytes) {
    unsigned char *d = dest;
    const unsigned char *s = source;
    if ((uintptr_t)d <= (uintptr_t)s) {
        for (size_t i=0; i<bytes; ++i) d[i]=s[i];
    } else {
        while (bytes) { --bytes; d[bytes]=s[bytes]; }
    }
    return dest;
}
void *memcpy(void *dest, const void *source, size_t bytes) {
    unsigned char *d = dest;
    const unsigned char *s = source;
    while (bytes--) *d++ = *s++;
    return dest;
}
void *memset(void *dest, int value, size_t bytes) {
    unsigned char *p = dest;
    while (bytes--) *p++ = (unsigned char)value;
    return dest;
}
#endif
int nxk_equal(const char *a, const char *b) {
    while (*a && *a == *b) { ++a; ++b; }
    return *a == *b;
}
