/* Pruebas kernel de paginas aisladas. Solo se enlaza en NXK_TEST.
 * No consume heap: la lista de reservas ocupa las propias paginas retenidas. */
#include "espacios.h"
#include "paginas.h"
#if defined(__i386__)
static int mismo_diagnostico(const struct nxk_diagnostico_paginas *a,
                             const struct nxk_diagnostico_paginas *b) {
    return a->total==b->total && a->libres==b->libres && a->prestadas==b->prestadas &&
        a->excluidas==b->excluidas && a->mayor_bloque==b->mayor_bloque &&
        a->bloques_libres==b->bloques_libres;
}
static int en_cero(uint32_t fisica,unsigned desde) {
    for (unsigned i=desde;i<4096;++i)
        if (((const unsigned char *)(uintptr_t)fisica)[i]) return 0;
    return 1;
}
/* Comprobar las cinco direcciones antes de usarlas como punteros: deben ser
 * exactamente las cinco paginas aisladas que se dejaron disponibles. */
static int paginas_esperadas(const struct nxk_espacio_info *info,const uint32_t *huecos) {
    uint32_t *d=(void *)(uintptr_t)info->directorio;
    int pertenece=0;
    for (unsigned i=0;i<5;++i) pertenece|=info->directorio==huecos[i];
    if (!pertenece) return 0;
    uint32_t fisicas[5]={info->directorio,d[0]&~4095u,d[1]&~4095u,
                        info->codigo_fisico,info->pila_fisica};
    for (unsigned i=0;i<5;++i) {
        pertenece=0;
        for (unsigned j=0;j<5;++j) pertenece|=fisicas[i]==huecos[j];
        if (!pertenece) return 0;
        for (unsigned j=0;j<i;++j)
            if (fisicas[i]==fisicas[j] || fisicas[i]+4096==fisicas[j] ||
                fisicas[j]+4096==fisicas[i]) return 0;
    }
    uint32_t *baja=(void *)(uintptr_t)fisicas[1],*alta=(void *)(uintptr_t)fisicas[2];
    if ((d[0]&7u)!=3 || (d[1]&7u)!=7 || baja[0] ||
        (alta[0]&~0x60u)!=(info->codigo_fisico|5u) ||
        (alta[1]&~0x60u)!=(info->pila_fisica|7u)) return 0;
    for (unsigned i=2;i<1024;++i) if ((d[i]|alta[i])&4u) return 0;
    for (unsigned i=0;i<1024;++i) if (baja[i]&4u) return 0;
    return en_cero(info->codigo_fisico,1) && en_cero(info->pila_fisica,0);
}
#endif
__attribute__((minsize)) int nxk_espacios_fragmentados_probar(void) {
#if defined(__i386__)
    struct nxk_diagnostico_paginas inicial,antes,despues;
    struct nxk_espacio_info info;
    uint32_t actual;
    __asm__ volatile("mov %%cr3,%0":"=r"(actual));
    if (actual!=0x200000 || nxk_paginas_diagnostico(&inicial)) return 1;
    /* La matriz de 32 MiB ejecuta el agotamiento; las mayores evitan otro
     * barrido con borrado de hasta 1 GiB. No se sustituye el mapa BIOS. */
    if (inicial.libres>8192) return 0;
    if (inicial.libres<12) return 2;
    for (unsigned i=0;i<NXK_ESPACIOS_MAX;++i)
        if (nxk_espacio_enumerar(i,&info)!=-2) return 3;
    size_t heap=nxk_heap_used();
    uint32_t huecos[6]={0},guardas[6]={0},cabeza=0,id=0,referencia=0,liberados=0;
    unsigned char codigo=0xc3,cruce[2];
    uint32_t valor=0x11223344,leido=0,dato=0;
    int fallo=4;
    if (nxk_espacio_crear(&codigo,1,&id)) goto limpiar;
    referencia=id;
    if (nxk_espacio_destruir(id)) goto limpiar;
    id=0;
    /* Conservar una pagina entre huecos impide que puedan formar un bloque. */
    for (unsigned i=0;i<6;++i) {
        huecos[i]=(uint32_t)(uintptr_t)nxk_pagina_reservar();
        guardas[i]=(uint32_t)(uintptr_t)nxk_pagina_reservar();
        if (!huecos[i] || !guardas[i]) goto limpiar;
    }
    void *pagina;
    while ((pagina=nxk_pagina_reservar())) {
        *(uint32_t *)pagina=cabeza; cabeza=(uint32_t)(uintptr_t)pagina;
    }
    fallo=5;
    for (unsigned i=0;i<5;++i) {
        if (nxk_paginas_diagnostico(&antes) || antes.libres!=i ||
            antes.mayor_bloque!=(i ? 1u : 0u) || antes.bloques_libres!=i) goto limpiar;
        uint32_t salida=0xdeadbeef;
        int r=nxk_espacio_crear(&codigo,1,&salida);
        if (!r) id=salida;
        if (r!=-4 || salida!=0xdeadbeef || nxk_paginas_diagnostico(&despues) ||
            !mismo_diagnostico(&antes,&despues) ||
            nxk_espacio_enumerar((referencia&255u)-1,&info)!=-2) goto limpiar;
        if (nxk_pagina_liberar((void *)(uintptr_t)huecos[i])) goto limpiar;
        liberados|=1u<<i;
    }
    fallo=6;
    if (nxk_paginas_diagnostico(&antes) || antes.libres!=5 ||
        antes.mayor_bloque!=1 || antes.bloques_libres!=5 ||
        nxk_espacio_crear(&codigo,1,&id) || id!=referencia+256 || nxk_paginas_libres() ||
        nxk_espacio_consultar(id,&info) || !paginas_esperadas(&info,huecos)) goto limpiar;
    fallo=7;
    if (nxk_espacio_escribir(id,NXK_ESPACIO_PILA,&valor,4) ||
        nxk_espacio_leer(id,NXK_ESPACIO_PILA-1,cruce,2) || cruce[0] || cruce[1]!=0x44 ||
        nxk_espacio_escribir(id,NXK_ESPACIO_CODIGO,&valor,1)!=-2 ||
        nxk_espacio_activar(id)) goto limpiar;
    if (*(volatile unsigned char *)NXK_ESPACIO_CODIGO!=codigo ||
        *(volatile uint32_t *)NXK_ESPACIO_PILA!=valor || nxk_espacio_destruir(id)!=-2) goto limpiar;
    *(volatile uint32_t *)(NXK_ESPACIO_PILA+4092)=0x88776655;
    if (nxk_espacio_restaurar() || nxk_espacio_leer(id,NXK_ESPACIO_PILA+4092,&leido,4) ||
        leido!=0x88776655) goto limpiar;
    fallo=8;
    if (nxk_pagina_liberar((void *)(uintptr_t)huecos[5])) goto limpiar;
    liberados|=1u<<5;
    if (nxk_espacio_reservar_pagina(id,&dato) || dato!=NXK_ESPACIO_DATOS ||
        nxk_paginas_libres() || !en_cero(huecos[5],0) ||
        nxk_espacio_escribir(id,dato,&valor,4) || nxk_espacio_proteger_paginas(id,dato,1,0) ||
        nxk_espacio_escribir(id,dato,&leido,4)!=-2 || nxk_espacio_limitar_datos(id,1)) goto limpiar;
    /* Refrescar dos veces debe conservar la PTE RO y su direccion fisica. */
    for (unsigned i=0;i<2;++i) {
        if (nxk_espacio_activar(id)) goto limpiar;
        uint32_t *d=(void *)(uintptr_t)info.directorio;
        uint32_t *alta=(void *)(uintptr_t)(d[1]&~4095u);
        if ((alta[2]&~0x60u)!=(huecos[5]|5u) ||
            *(volatile uint32_t *)dato!=valor || nxk_espacio_restaurar()) goto limpiar;
    }
    fallo=9;
    referencia=id;
    if (nxk_espacio_destruir(id)) goto limpiar;
    id=0;
    for (unsigned i=0;i<6;++i) if (!en_cero(huecos[i],0)) goto limpiar;
    if (nxk_paginas_diagnostico(&despues) || despues.libres!=6 ||
        despues.mayor_bloque!=1 || despues.bloques_libres!=6 ||
        nxk_espacio_crear(&codigo,1,&id) || id!=referencia+256 ||
        nxk_espacio_consultar(id,&info) || info.paginas_datos || info.cuota_datos!=4 ||
        !paginas_esperadas(&info,huecos) || nxk_espacio_consultar(referencia,&info)!=-1) goto limpiar;
    fallo=0;
limpiar:
    if (nxk_espacio_restaurar()) return 10;
    if (id && nxk_espacio_destruir(id)) fallo=11;
    for (unsigned i=0;i<6;++i) {
        if (huecos[i] && !(liberados&(1u<<i)) &&
            nxk_pagina_liberar((void *)(uintptr_t)huecos[i])) fallo=12;
        if (guardas[i] && nxk_pagina_liberar((void *)(uintptr_t)guardas[i])) fallo=12;
    }
    while (cabeza) {
        uint32_t siguiente=*(uint32_t *)(uintptr_t)cabeza;
        if (nxk_pagina_liberar((void *)(uintptr_t)cabeza)) fallo=12;
        cabeza=siguiente;
    }
    if (nxk_paginas_diagnostico(&despues) || !mismo_diagnostico(&inicial,&despues) ||
        nxk_heap_used()!=heap) fallo=13;
    return fallo;
#else
    return 0;
#endif
}
