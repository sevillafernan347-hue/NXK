/* Demostracion y pruebas de ejecucion CPL3 con espacios conservados.
 * Puente C sobre entrada/salida NEXTRY ASM. Una CPU, llamadas desde consola.
 * Cada turno empieza en codigo400000/ESP402000; no reanuda registros anteriores. */
#include "usuario.h"
#include "espacios.h"
#include "paginas.h"
#include "paginacion.h"
#include "tareas.h"
#if defined(__i386__)
static uint32_t cr3_actual(void) {
    uint32_t cr3; __asm__ volatile("mov %%cr3,%0":"=r"(cr3)); return cr3;
}
static int contexto_demo(void) {
    uint32_t flags; __asm__ volatile("pushfl; popl %0":"=r"(flags));
    return cr3_actual()==0x200000u && (flags&0x200u) && !nxk_tarea_actual();
}
static int normal(uint32_t id,uint32_t salida) {
    struct nxk_usuario_resultado r;
    return nxk_usuario_ejecutar_espacio(id,&r) || r.fallo || r.salida!=salida ||
        r.vector || r.error || r.direccion;
}
/* No dejar reservas al fallar una comprobacion o al encontrar el pool lleno. */
static int limpiar(uint32_t *ids,unsigned n) {
    int r=nxk_espacio_restaurar();
    for (unsigned i=0;i<n;++i)
        if (ids[i] && nxk_espacio_destruir(ids[i])) r=-1;
    return r;
}
int nxk_usuario_espacios_demo(void) {
    if (!contexto_demo()) return -2;
    /* A/B conservan contadores distintos en la misma direccion virtual. */
    static const unsigned char a[]={0xa1,0,0x10,0x40,0,0x83,0xc0,1,0xa3,0,0x10,0x40,0,0xcd,0x80};
    static const unsigned char b[]={0xa1,0,0x10,0x40,0,0x83,0xc0,10,0xa3,0,0x10,0x40,0,0xcd,0x80};
    static const unsigned char c[]={0xc7,5,0,0,0x40,0,1,0,0,0,0xcd,0x80};
    static const unsigned char d[]={0xeb,0xfe};
    const unsigned char *codigos[]={a,b,c,d};
    const size_t longitudes[]={sizeof(a),sizeof(b),sizeof(c),sizeof(d)};
    uint32_t ids[4]={0}, libres=nxk_paginas_libres(), guardas=nxk_paginacion_guardas();
    int fallo=0; struct nxk_usuario_resultado r;
    for (unsigned i=0;i<4;++i)
        if (nxk_espacio_crear(codigos[i],longitudes[i],&ids[i])) { fallo=1; break; }
    if (!fallo && (normal(ids[0],1) || normal(ids[1],10) || normal(ids[0],2))) fallo=2;
    if (!fallo && (nxk_usuario_ejecutar_espacio(ids[2],&r) || !r.fallo ||
        r.vector!=14 || r.error!=7 || r.direccion!=NXK_ESPACIO_CODIGO)) fallo=3;
    if (!fallo && (nxk_usuario_ejecutar_espacio(ids[3],&r) || !r.fallo ||
        r.vector!=32 || r.error || r.direccion)) fallo=4;
    if (!fallo && normal(ids[0],3)) fallo=5;
    if (!fallo) for (unsigned i=0;i<4;++i) {
        uint32_t dato;
        if (nxk_espacio_leer(ids[i],NXK_ESPACIO_PILA,&dato,sizeof(dato)) ||
            dato!=(i==0 ? 3u : i==1 ? 10u : 0u)) { fallo=6; break; }
    }
    if (!fallo) {
        unsigned char codigo[sizeof(c)];
        if (nxk_espacio_leer(ids[2],NXK_ESPACIO_CODIGO,codigo,sizeof(codigo))) fallo=7;
        for (unsigned i=0;!fallo && i<sizeof(codigo);++i) if (codigo[i]!=c[i]) fallo=7;
    }
    if (limpiar(ids,4)) fallo=8;
    if (nxk_paginas_libres()!=libres || nxk_paginacion_guardas()!=guardas || cr3_actual()!=0x200000u) fallo=9;
    return fallo ? -fallo : 0;
}
#ifdef NXK_TEST
static int intacto(const struct nxk_usuario_resultado *r) {
    const unsigned char *p=(const void *)r;
    for (unsigned i=0;i<sizeof(*r);++i) if (p[i]!=0xa5) return 0;
    return 1;
}
int nxk_usuario_espacios_probar(void) {
    if (!contexto_demo()) return 1;
    uint32_t id=0, libres=nxk_paginas_libres(), guardas=nxk_paginacion_guardas();
    struct nxk_usuario_resultado r; memset(&r,0xa5,sizeof(r));
    size_t heap=nxk_heap_used(); unsigned historial=nxk_usuario_historial_cantidad();
    /* Rechazos previos a ejecutar no consumen heap ni publican resultados. */
    if (nxk_usuario_ejecutar_espacio(0,&r)!=-1 || !intacto(&r) ||
        nxk_heap_used()!=heap || nxk_usuario_historial_cantidad()!=historial) return 2;
    static const unsigned char salida_alta[]={0xb8,0xff,0xff,0xff,0xff,0xcd,0x80};
    if (nxk_espacio_crear(salida_alta,sizeof(salida_alta),&id)) return 3;
    int fallo=0;
    if (nxk_usuario_ejecutar_espacio(id,0)!=-1 || nxk_heap_used()!=heap) fallo=4;
    if (!fallo) {
        uint32_t flags; __asm__ volatile("pushfl; popl %0; cli":"=r"(flags)::"memory");
        int rechazo=nxk_usuario_ejecutar_espacio(id,&r);
        __asm__ volatile("pushl %0; popfl"::"r"(flags):"memory","cc");
        if (rechazo!=-2 || !intacto(&r) || nxk_heap_used()!=heap) fallo=5;
    }
    if (!fallo) {
        if (nxk_espacio_activar(id)) fallo=6;
        else {
            int rechazo=nxk_usuario_ejecutar_espacio(id,&r);
            if (nxk_espacio_restaurar() || rechazo!=-2 || !intacto(&r)) fallo=7;
        }
    }
    /* Un EAX con bit31 no es un fallo: la autoridad es el campo fallo. */
    if (!fallo && normal(id,UINT32_MAX)) fallo=8;
    if (limpiar(&id,1)) fallo=9;
    if (!fallo) {
        memset(&r,0xa5,sizeof(r)); heap=nxk_heap_used();
        if (nxk_usuario_ejecutar_espacio(id,&r)!=-1 || !intacto(&r) || nxk_heap_used()!=heap) fallo=10;
    }
    /* La primera ejecucion reserva una pila kernel de retorno reutilizable. */
    if (!fallo && nxk_usuario_espacios_demo()) fallo=11;
    heap=nxk_heap_used();
    if (!fallo && (nxk_usuario_espacios_demo() || nxk_usuario_espacios_demo() || nxk_heap_used()!=heap)) fallo=12;
    if (nxk_paginas_libres()!=libres || nxk_paginacion_guardas()!=guardas || cr3_actual()!=0x200000u) fallo=13;
    return fallo;
}
#endif
#else
int nxk_usuario_espacios_demo(void) { return -64; }
#endif
