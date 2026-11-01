/* Mapa de memoria del BIOS recogido por boot/arranque32.nxa.
 * Este puente C informa RAM descrita como utilizable; no asigna paginas.
 * No equivale a memoria libre: incluye regiones ocupadas por NXK.
 * Se migrara a NEXTRY cuando el lenguaje permita estructuras y punteros.
 */
#include "nxk.h"
struct __attribute__((packed)) region_bios {
    uint64_t base, longitud;
    uint32_t tipo;
};
uint32_t nxk_ram_kib(void) {
#if defined(__i386__)
    volatile const uint16_t *cabecera=(const uint16_t *)0x5000;
    volatile const struct region_bios *regiones=(const void *)0x5010;
    unsigned cantidad=cabecera[0];
    if (cabecera[1]!=1 || !cantidad || cantidad>32) return 0;
    uint64_t total=0;
    for (unsigned i=0;i<cantidad;++i) {
        uint64_t base=regiones[i].base, longitud=regiones[i].longitud;
        if (longitud>UINT64_MAX-base) return 0;
        /* Solo RAM de tipo 1 por debajo de 4 GiB para nuestro objetivo x86. */
        if (regiones[i].tipo!=1 || base>=0x100000000ull) continue;
        uint64_t limite=0x100000000ull-base;
        total+=longitud<limite ? longitud : limite;
        if (total>0x100000000ull) return 0;
    }
    return (uint32_t)(total>>10);
#else
    return 0; /* El arranque de 64 bits aun no proporciona este mapa. */
#endif
}
