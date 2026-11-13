/* Casos sinteticos y agotamiento real. Solo se enlaza en la imagen de pruebas. */
#include "paginas.h"
__attribute__((minsize)) int nxk_probar_paginas(void) {
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
    /* Reserva parcial redondeada: repeticion no descuenta paginas dos veces. */
    mapa[1]=(struct nxk_region_ram){0x608000,0x5f8000,1};
    if (nxk_paginas_iniciar(mapa,2) || nxk_paginas_total()!=1024) return 19;
    if (nxk_paginas_reservar_region(0x803001,4095) || nxk_paginas_libres()!=1023) return 20;
    if (nxk_paginas_reservar_region(0x803001,4095) || nxk_paginas_libres()!=1023) return 21;
    void *activa=nxk_pagina_reservar();
    uint32_t antes_reserva=nxk_paginas_libres();
    if ((uintptr_t)activa!=0x800000) return 22;
    if (nxk_paginas_reservar_region(0x800fff,1)!=-2) return 24;
    if (nxk_paginas_libres()!=antes_reserva) return 25;
    if (nxk_pagina_liberar(activa)) return 26;
    if (nxk_paginas_reservar_region(0x3fffffff,2)!=-1 ||
        nxk_paginas_reservar_region(0,0) || nxk_paginas_libres()!=1023) return 23;
    /* Bloque alineado: no debe cruzar la pagina reservada 0x803000. */
    void *bloque=nxk_paginas_reservar_bloque(4,4);
    if ((uintptr_t)bloque!=0x804000 || nxk_paginas_libres()!=1019) return 27;
    memset(bloque,0x7b,4*4096);
    uint32_t libres_antes=nxk_paginas_libres();
    if (nxk_paginas_reservar_bloque(0,1) || nxk_paginas_reservar_bloque(1,3) ||
        nxk_paginas_reservar_bloque(UINT32_MAX,1) || nxk_paginas_libres()!=libres_antes) return 28;
    if (nxk_paginas_liberar_bloque(bloque,5)!=-2 || nxk_paginas_libres()!=libres_antes) return 29;
    for (unsigned i=0;i<4*4096;++i) if (((unsigned char *)bloque)[i]!=0x7b) return 30;
    if (nxk_paginas_liberar_bloque(bloque,4) || nxk_paginas_libres()!=1023 ||
        nxk_paginas_liberar_bloque(bloque,4)!=-2) return 31;
    for (unsigned i=0;i<4*4096;++i) if (((unsigned char *)bloque)[i]) return 32;
    if (nxk_paginas_liberar_bloque((void *)0x3ffff000,2)!=-1) return 33;
    /* Muchas paginas libres pero ninguna corrida de 1021 paginas. */
    if (nxk_paginas_reservar_bloque(1021,1) || nxk_paginas_libres()!=1023) return 34;
    /* La union de fragmentos no alineados cubre una pagina completa, aunque
       las entradas lleguen desordenadas y haya una reserva intercalada. */
    struct nxk_region_ram partes[]={
        {0x800800,0x3ff800,1}, {0x600000,0x123,1},
        {0x801900,1,2}, {0x600123,0x2006dd,1}
    };
    if (nxk_paginas_iniciar(partes,4) || nxk_paginas_total()!=1023) return 35;
    if ((uintptr_t)(a=nxk_pagina_reservar())!=0x800000 || nxk_pagina_liberar(a)) return 36;
    /* Un hueco de un solo byte dentro de metadatos impide la inicializacion. */
    partes[3].base++; partes[3].longitud--;
    if (nxk_paginas_iniciar(partes,4)!=-4 || nxk_paginas_total()!=1023) return 37;
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
