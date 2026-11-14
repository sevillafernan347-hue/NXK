/* Casos sinteticos y agotamiento real. Solo se enlaza en la imagen de pruebas. */
#include "paginas.h"
/* Un error debe conservar cada byte de la salida, no publicar un resumen parcial. */
static int diagnostico_error(int esperado) {
    struct nxk_diagnostico_paginas d;
    memset(&d,0xa5,sizeof(d));
    if (nxk_paginas_diagnostico(&d)!=esperado) return 1;
    for (unsigned i=0;i<sizeof(d);++i)
        if (((unsigned char *)&d)[i]!=0xa5) return 1;
    return 0;
}
int nxk_probar_paginas_preinicio(void) {
    uint32_t pagina=0x800000;
#if defined(__i386__)
    return diagnostico_error(-3) || nxk_paginas_diagnostico(0)!=-1 ||
        nxk_paginas_liberar_lista(&pagina,1)!=-3;
#else
    return diagnostico_error(-64) || nxk_paginas_diagnostico(0)!=-64 ||
        nxk_paginas_liberar_lista(&pagina,1)!=-64 || nxk_paginas_liberar_lista(0,0)!=-64;
#endif
}
#if defined(__i386__)
static int diagnostico_igual(uint32_t total,uint32_t libres,uint32_t prestadas,
                             uint32_t excluidas,uint32_t mayor,uint32_t bloques) {
    /* Repetir confirma que consultar no consume paginas ni altera el resumen. */
    for (unsigned i=0;i<2;++i) {
        struct nxk_diagnostico_paginas d;
        if (nxk_paginas_diagnostico(&d) || d.total!=total || d.libres!=libres ||
            d.prestadas!=prestadas || d.excluidas!=excluidas ||
            d.mayor_bloque!=mayor || d.bloques_libres!=bloques) return 1;
    }
    return 0;
}
/* Miembros separados, validacion completa y lista alojada en memoria que se
 * libera. Solo se invoca con el mapa BIOS restaurado y RAM real disponible. */
__attribute__((minsize)) static int probar_lista(void) {
    uint32_t paginas[32],lista[32];
    struct nxk_diagnostico_paginas antes,actual;
    if (nxk_paginas_diagnostico(&antes)) return 1;
    for (unsigned i=0;i<32;++i) {
        paginas[i]=(uint32_t)(uintptr_t)nxk_pagina_reservar();
        if (!paginas[i]) return 2;
        memset((void *)(uintptr_t)paginas[i],(int)(i+1),4096);
    }
    /* Dejar libre el ultimo miembro: el primero prestado no debe borrarse
     * cuando la validacion llega al segundo y descubre que no esta prestado. */
    lista[0]=paginas[31];
    if (nxk_paginas_liberar_lista(lista,1)) return 3;
    lista[0]=paginas[0]; lista[1]=paginas[31];
    if (nxk_paginas_liberar_lista(lista,2)!=-2 ||
        nxk_paginas_liberar_lista(0,1)!=-1 ||
        nxk_paginas_liberar_lista(lista,0)!=-1 ||
        nxk_paginas_liberar_lista(lista,33)!=-1 ||
        nxk_paginas_liberar_lista(lista,UINT32_MAX)!=-1) return 4;
    uint32_t invalidas[]={0,0x7ff000,0x40000000,UINT32_MAX,paginas[0]+1,paginas[0]};
    for (unsigned i=0;i<sizeof(invalidas)/sizeof(*invalidas);++i) {
        lista[1]=invalidas[i];
        if (nxk_paginas_liberar_lista(lista,2)!=-1) return 5;
    }
    if (nxk_paginas_diagnostico(&actual) || actual.libres!=antes.libres-31 ||
        actual.prestadas!=antes.prestadas+31 || actual.excluidas!=antes.excluidas) return 6;
    for (unsigned i=0;i<31;++i) for (unsigned j=0;j<4096;++j)
        if (((unsigned char *)(uintptr_t)paginas[i])[j]!=i+1) return 7;
    /* Desordenadas y separadas por pagina[1], que debe conservar su contenido. */
    lista[0]=paginas[2]; lista[1]=paginas[0];
    if (nxk_paginas_liberar_lista(lista,2) || nxk_paginas_liberar_lista(lista,2)!=-2) return 8;
    for (unsigned j=0;j<4096;++j)
        if (((unsigned char *)(uintptr_t)paginas[0])[j] ||
            ((unsigned char *)(uintptr_t)paginas[2])[j] ||
            ((unsigned char *)(uintptr_t)paginas[1])[j]!=2) return 9;
    lista[0]=paginas[1];
    for (unsigned i=3;i<31;++i) lista[i-2]=paginas[i];
    if (nxk_paginas_liberar_lista(lista,29) ||
        diagnostico_igual(antes.total,antes.libres,antes.prestadas,antes.excluidas,
                          antes.mayor_bloque,antes.bloques_libres)) return 10;
    /* Limite de 32 miembros. La propia lista esta en la primera pagina que
     * se borra: leerla de nuevo tras borrar produciria direcciones incorrectas. */
    for (unsigned i=0;i<32;++i) {
        paginas[i]=(uint32_t)(uintptr_t)nxk_pagina_reservar();
        if (!paginas[i]) return 11;
        memset((void *)(uintptr_t)paginas[i],0x7b,4096);
    }
    uint32_t *interna=(void *)(uintptr_t)(paginas[0]+64);
    interna[0]=paginas[0];
    for (unsigned i=1;i<32;++i) interna[i]=paginas[32-i];
    if (nxk_paginas_liberar_lista(interna,32)) return 12;
    for (unsigned i=0;i<32;++i) for (unsigned j=0;j<4096;++j)
        if (((unsigned char *)(uintptr_t)paginas[i])[j]) return 13;
    return diagnostico_igual(antes.total,antes.libres,antes.prestadas,antes.excluidas,
                            antes.mayor_bloque,antes.bloques_libres) ? 14 : 0;
}
#endif
__attribute__((minsize)) int nxk_probar_paginas(void) {
#if defined(__i386__)
    struct nxk_region_ram mapa[]={
        {0x600000,0x600000,1}, /* RAM hasta 12 MiB. */
        {0x800000+100,100,2}, /* Reserva parcial: debe excluir la pagina entera. */
        {0x900000,0x100000,1} /* Duplicado: no debe contar RAM dos veces. */
    };
    if (nxk_paginas_iniciar(mapa,3) || nxk_paginas_total()!=1023) return 1;
    if (nxk_paginas_diagnostico(0)!=-1 ||
        diagnostico_igual(1023,1023,0,0,1023,1)) return 38;
    void *a=nxk_pagina_reservar(), *b=nxk_pagina_reservar();
    if ((uintptr_t)a!=0x801000 || (uintptr_t)b!=0x802000) return 2;
    if (diagnostico_igual(1023,1021,2,0,1021,1)) return 39;
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
    if (diagnostico_igual(1023,1023,0,0,1023,1)) return 40;
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
    if (diagnostico_igual(1024,1024,0,0,1024,1)) return 41;
    /* Inyectar corrupcion solo en esta imagen de pruebas. Restaurar siempre
     * los bytes antes de cualquier operacion del asignador, incluso si falla. */
    volatile unsigned char *libre=(void *)(0x600000+0x800000/4096/8);
    volatile unsigned char *prestada=(void *)(0x608000+0x800000/4096/8);
    unsigned char guardado=*prestada;
    *prestada=(unsigned char)(guardado|1);
    int error=diagnostico_error(-2);
    *prestada=guardado;
    if (error) return 42;
    guardado=*libre;
    *libre=(unsigned char)(guardado&~1u);
    error=diagnostico_error(-2);
    *libre=guardado;
    if (error || diagnostico_igual(1024,1024,0,0,1024,1)) return 43;
    if (nxk_paginas_reservar_region(0x803001,4095) || nxk_paginas_libres()!=1023) return 20;
    if (nxk_paginas_reservar_region(0x803001,4095) || nxk_paginas_libres()!=1023) return 21;
    if (diagnostico_igual(1024,1023,0,1,1020,2)) return 44;
    void *activa=nxk_pagina_reservar();
    uint32_t antes_reserva=nxk_paginas_libres();
    if ((uintptr_t)activa!=0x800000) return 22;
    if (nxk_paginas_reservar_region(0x800fff,1)!=-2) return 24;
    if (nxk_paginas_libres()!=antes_reserva) return 25;
    if (diagnostico_igual(1024,1022,1,1,1020,2)) return 45;
    if (nxk_pagina_liberar(activa)) return 26;
    if (nxk_paginas_reservar_region(0x3fffffff,2)!=-1 ||
        nxk_paginas_reservar_region(0,0) || nxk_paginas_libres()!=1023) return 23;
    /* Bloque alineado: no debe cruzar la pagina reservada 0x803000. */
    void *bloque=nxk_paginas_reservar_bloque(4,4);
    if ((uintptr_t)bloque!=0x804000 || nxk_paginas_libres()!=1019) return 27;
    if (diagnostico_igual(1024,1019,4,1,1016,2)) return 46;
    memset(bloque,0x7b,4*4096);
    uint32_t libres_antes=nxk_paginas_libres();
    if (nxk_paginas_reservar_bloque(0,1) || nxk_paginas_reservar_bloque(1,3) ||
        nxk_paginas_reservar_bloque(UINT32_MAX,1) || nxk_paginas_libres()!=libres_antes) return 28;
    if (nxk_paginas_liberar_bloque(bloque,5)!=-2 || nxk_paginas_libres()!=libres_antes) return 29;
    if (diagnostico_igual(1024,1019,4,1,1016,2)) return 47;
    for (unsigned i=0;i<4*4096;++i) if (((unsigned char *)bloque)[i]!=0x7b) return 30;
    if (nxk_paginas_liberar_bloque(bloque,4) || nxk_paginas_libres()!=1023 ||
        nxk_paginas_liberar_bloque(bloque,4)!=-2) return 31;
    for (unsigned i=0;i<4*4096;++i) if (((unsigned char *)bloque)[i]) return 32;
    if (nxk_paginas_liberar_bloque((void *)0x3ffff000,2)!=-1) return 33;
    /* Muchas paginas libres pero ninguna corrida de 1021 paginas. */
    if (nxk_paginas_reservar_bloque(1021,1) || nxk_paginas_libres()!=1023) return 34;
    if (diagnostico_igual(1024,1023,0,1,1020,2)) return 48;
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
    if (diagnostico_igual(1023,1023,0,0,1022,2)) return 49;
    /* Metadatos validos pero ninguna pagina asignable: no es un error. */
    struct nxk_region_ram bordes[]={{0x600000,0x10000,1},{0x3ffff000,4096,1}};
    if (nxk_paginas_iniciar(bordes,1) || diagnostico_igual(0,0,0,0,0,0)) return 50;
    /* La corrida que termina exactamente en 1 GiB tambien debe contarse.
     * Solo se consultan bits: no se accede a RAM inexistente en QEMU de 32 MiB. */
    if (nxk_paginas_iniciar(bordes,2) || diagnostico_igual(1,1,0,0,1,1)) return 51;
    if (nxk_paginas_reservar_region(0x3ffff000,4096) ||
        diagnostico_igual(1,0,0,1,0,0)) return 52;
    /* Restaurar el mapa real; agotar y liberar todas sus paginas sin escribirlas. */
    if (nxk_paginas_bios() || !nxk_paginas_total()) return 10;
    uint32_t cantidad=nxk_paginas_total();
    struct nxk_diagnostico_paginas inicial;
    if (nxk_paginas_diagnostico(&inicial)) return 53;
    uintptr_t anterior=0;
    for (uint32_t i=0;i<cantidad;++i) {
        uintptr_t p=(uintptr_t)nxk_pagina_reservar();
        if (p<0x800000 || p>=0x40000000 || (p&4095) || p<=anterior) return 11;
        anterior=p;
    }
    if (nxk_pagina_reservar() || nxk_paginas_libres()) return 12;
    if (diagnostico_igual(cantidad,0,cantidad,0,0,0)) return 54;
    uint32_t liberadas=0;
    for (uintptr_t p=0x800000;p<0x40000000;p+=4096)
        if (!nxk_pagina_liberar((void *)p)) ++liberadas;
    if (liberadas!=cantidad || nxk_paginas_libres()!=cantidad) return 13;
    if (diagnostico_igual(cantidad,cantidad,0,0,
                         inicial.mayor_bloque,inicial.bloques_libres)) return 55;
    int lista=probar_lista(); if (lista) return 60+lista;
#else
    if (nxk_probar_paginas_preinicio()) return 56;
#endif
    return 0;
}
