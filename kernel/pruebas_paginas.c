/* Casos sinteticos y agotamiento real. Solo se enlaza en la imagen de pruebas. */
#include "paginas.h"
int nxk_probar_paginas(void) {
#if defined(__i386__)
    struct nxk_region_ram mapa[]={
        {0x600000,0x600000,1}, /* RAM hasta 12 MiB. */
        {0x800000+100,100,2}, /* Reserva parcial: debe excluir la pagina entera. */
        {0x900000,0x100000,1} /* Duplicado: no debe contar RAM dos veces. */
    };
    if (nxk_paginas_iniciar(mapa,3) || nxk_paginas_total()!=1023) return 1;
    void *a=nxk_pagina_reservar(), *b=nxk_pagina_reservar();
    if ((uintptr_t)a!=0x801000 || (uintptr_t)b!=0x802000) return 2;
    for (unsigned i=0;i<4096;++i)
        if (((unsigned char *)a)[i] || ((unsigned char *)b)[i]) return 16;
    /* Escrituras reales para comprobar que las paginas son independientes. */
    memset(a,0x35,4096); memset(b,0x79,4096);
    for (unsigned i=0;i<4096;++i)
        if (((unsigned char *)a)[i]!=0x35 || ((unsigned char *)b)[i]!=0x79) return 3;
    if (!nxk_paginas_iniciar(mapa,3)) return 4;
    if (!nxk_pagina_liberar((void *)0x800000) || !nxk_pagina_liberar((char *)a+1)) return 5;
    if (nxk_pagina_liberar(a)) return 6;
    for (unsigned i=0;i<4096;++i) if (((unsigned char *)a)[i]) return 18;
    if (!nxk_pagina_liberar(a) || nxk_pagina_reservar()!=a) return 6;
    for (unsigned i=0;i<4096;++i) if (((unsigned char *)a)[i]) return 17;
    if (nxk_pagina_liberar(a) || nxk_pagina_liberar(b)) return 7;
    mapa[0].base=UINT64_MAX-10; mapa[0].longitud=20;
    if (!nxk_paginas_iniciar(mapa,3) || nxk_paginas_libres()!=1023) return 8;
    mapa[0]=(struct nxk_region_ram){0x600000,0x600000,1};
    mapa[1]=(struct nxk_region_ram){0x600001,1,2};
    if (!nxk_paginas_iniciar(mapa,3)) return 9;
    /* Dos regiones contiguas pueden cubrir juntas la reserva de metadatos. */
    mapa[0]=(struct nxk_region_ram){0x600000,0x8000,1};
    mapa[1]=(struct nxk_region_ram){0x608000,0x5f8000,1};
    if (nxk_paginas_iniciar(mapa,2) || nxk_paginas_total()!=1024) return 14;
    /* Un hueco de pagina dentro de la reserva invalida el mapa. */
    mapa[1]=(struct nxk_region_ram){0x609000,0x5f7000,1};
    if (nxk_paginas_iniciar(mapa,2)!=-4) return 15;
    /* Restaurar el mapa real; agotar y liberar todas sus paginas sin escribirlas. */
    if (nxk_paginas_bios() || !nxk_paginas_total()) return 10;
    uint32_t cantidad=nxk_paginas_total();
    uintptr_t anterior=0;
    for (uint32_t i=0;i<cantidad;++i) {
        uintptr_t p=(uintptr_t)nxk_pagina_reservar();
        if (p<0x800000 || p>=0x40000000 || (p&4095) || p<=anterior) return 11;
        anterior=p;
    }
    if (nxk_pagina_reservar() || nxk_paginas_libres()) return 12;
    uint32_t liberadas=0;
    for (uintptr_t p=0x800000;p<0x40000000;p+=4096)
        if (!nxk_pagina_liberar((void *)p)) ++liberadas;
    if (liberadas!=cantidad || nxk_paginas_libres()!=cantidad) return 13;
#endif
    return 0;
}
