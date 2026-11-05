/* Resumen del mapa BIOS E820. RAM descrita no equivale a RAM libre;
 * incluye memoria ocupada por NXK. Puente C hasta admitir estructuras NEXTRY. */
#include "nxk.h"
struct __attribute__((packed)) region_bios { uint64_t base, longitud; uint32_t tipo; };
#if defined(__i386__)
/* Barrer extremos permite unir RAM y excluir reservas sin memoria dinamica.
 * Cualquier tipo distinto de 1 prevalece sobre las regiones utilizables. */
static uint32_t contar_ram(volatile const struct region_bios *r,unsigned n) {
    const uint64_t fin=0x100000000ull;
    if (!n || n>32) return 0;
    for (unsigned i=0;i<n;++i)
        if (r[i].longitud>UINT64_MAX-r[i].base) return 0;
    uint64_t posicion=0,total=0;
    while (posicion<fin) {
        uint64_t siguiente=fin; int util=0,reservada=0;
        for (unsigned i=0;i<n;++i) {
            uint64_t base=r[i].base, limite=base+r[i].longitud;
            if (base>posicion && base<siguiente) siguiente=base;
            if (limite>posicion && limite<siguiente) siguiente=limite;
            if (base<=posicion && posicion<limite) {
                if (r[i].tipo==1) util=1; else reservada=1;
            }
        }
        if (util && !reservada) total+=siguiente-posicion;
        posicion=siguiente;
    }
    return (uint32_t)(total>>10);
}
#ifdef NXK_TEST
int nxk_ram_probar(void) {
    struct region_bios r[]={ {0,0x4000,1}, {0x2000,0x4000,1},
        {0x3000,0x1000,2}, {0x100000000ull,0x1000,1} };
    if (contar_ram(r,4)!=20 || contar_ram(r,0)!=0 || contar_ram(r,33)!=0) return 1;
    r[0].base=0; r[0].longitud=0x100000000ull;
    if (contar_ram(r,1)!=4194304u) return 2;
    r[0].base=UINT64_MAX-2; r[0].longitud=3;
    if (contar_ram(r,1)!=0) return 3;
    r[0].base=0x100000000ull-1024; r[0].longitud=2048;
    if (contar_ram(r,1)!=1) return 4;
    r[0]=(struct region_bios){0x800,0x800,1};
    r[1]=(struct region_bios){0,0x800,1};
    if (contar_ram(r,2)!=4) return 5;
    return 0;
}
#endif
#endif
uint32_t nxk_ram_kib(void) {
#if defined(__i386__)
    volatile const uint16_t *cabecera=(const uint16_t *)0x5000;
    if (cabecera[1]!=1) return 0;
    return contar_ram((const void *)0x5010,cabecera[0]);
#else
    return 0; /* El arranque x64 no proporciona todavia el mapa. */
#endif
}
