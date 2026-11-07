/* Esperas temporizadas de procesos CPL3: no girar en usuario ni consumir
 * turnos mientras duermen. Puente C sobre entrada/reanudacion NEXTRY ASM. */
#include "procesos.h"
#include "paginas.h"
#include "paginacion.h"
#include "tareas.h"
#if defined(__i386__)
extern uint32_t nxk_reloj_ticks(void);
static uint32_t cr3_actual(void) {
    uint32_t n; __asm__ volatile("mov %%cr3,%0":"=r"(n)); return n;
}
static int contexto_demo(void) {
    uint32_t flags,cr0;
    __asm__ volatile("pushfl; popl %0":"=r"(flags));
    __asm__ volatile("mov %%cr0,%0":"=r"(cr0));
    return (cr0&0x80000000u) && cr3_actual()==0x200000u &&
        (flags&0x200u) && !nxk_tarea_actual();
}
static int vencido(uint32_t ahora,uint32_t plazo) { return ahora-plazo<0x80000000u; }
/* Dormir20 conserva EBX solicitado, ESI y sello/ESP; al volver EAX debe ser0. */
static const unsigned char dormilon[]={
    0xbb,0x14,0x00,0x00,0x00,0xbe,0x10,0x32,0x54,0x76,0x68,0x78,
    0x56,0x34,0x12,0xb8,0x0a,0x00,0x00,0x00,0xcd,0x81,0x85,0xc0,
    0x0f,0x85,0x38,0x00,0x00,0x00,0x81,0xfb,0x14,0x00,0x00,0x00,
    0x0f,0x85,0x2c,0x00,0x00,0x00,0x81,0xfe,0x10,0x32,0x54,0x76,
    0x0f,0x85,0x20,0x00,0x00,0x00,0x81,0xfc,0xfc,0x1f,0x40,0x00,
    0x0f,0x85,0x14,0x00,0x00,0x00,0x5a,0x81,0xfa,0x78,0x56,0x34,
    0x12,0x0f,0x85,0x07,0x00,0x00,0x00,0xb8,0x2a,0x00,0x00,0x00,
    0xcd,0x80,0x0f,0x0b
};
static const unsigned char vecino[]={0xb8,7,0,0,0,0xcd,0x80};
static int limpiar(uint32_t *ids,unsigned n) {
    int r=nxk_espacio_restaurar();
    for (unsigned i=0;i<n;++i) if (ids[i] && nxk_proceso_liberar(ids[i])) r=-1;
    return r;
}
int nxk_proceso_dormir_demo(void) {
    if (!contexto_demo()) return -1;
    /* No despertar ni ejecutar procesos ajenos desde la demostracion. */
    for (unsigned i=0;i<NXK_ESPACIOS_MAX;++i) {
        struct nxk_proceso_info p; if (!nxk_proceso_enumerar(i,&p)) return -2;
    }
    uint32_t ids[2]={0}, libres=nxk_paginas_libres(), guardas=nxk_paginacion_guardas();
    int fallo=0; struct nxk_proceso_info a,b;
    if (nxk_proceso_crear(dormilon,sizeof(dormilon),&ids[0]) ||
        nxk_proceso_crear(vecino,sizeof(vecino),&ids[1])) fallo=3;
    if (!fallo && (nxk_procesos_ejecutar(2)!=2 ||
        nxk_proceso_consultar(ids[0],&a) || nxk_proceso_consultar(ids[1],&b) ||
        a.estado!=NXK_PROCESO_DORMIDO || a.turnos!=1 || a.dormidas!=1 ||
        a.cesiones || a.preempciones || a.ultimo.fallo || a.error_motor ||
        b.estado!=NXK_PROCESO_TERMINADO || b.turnos!=1 || b.ultimo.fallo ||
        b.ultimo.salida!=7 || b.dormidas || b.error_motor)) fallo=4;
    if (!fallo && (nxk_proceso_paso(ids[0])!=-2 || nxk_procesos_ejecutar(8)!=2 ||
        nxk_proceso_consultar(ids[0],&a) || a.turnos!=1 || a.dormidas!=1 ||
        a.estado!=NXK_PROCESO_DORMIDO)) fallo=5;
    if (!fallo) {
        /* IRQ despierta HLT; no hacer consultas continuas ni ocupar la CPU.
         * El deadline0 es valido: el estado, no su valor, indica la espera. */
        while (!vencido(nxk_reloj_ticks(),a.despertar)) __asm__ volatile("hlt");
        if (nxk_procesos_ejecutar(2) || nxk_proceso_consultar(ids[0],&a) ||
            a.estado!=NXK_PROCESO_TERMINADO || a.turnos!=2 || a.dormidas!=1 ||
            a.cesiones || a.ultimo.fallo || a.ultimo.salida!=42 || a.despertar ||
            a.error_motor) fallo=6;
    }
    if (limpiar(ids,2)) fallo=7;
    if (nxk_paginas_libres()!=libres || nxk_paginacion_guardas()!=guardas ||
        cr3_actual()!=0x200000u) fallo=8;
    return fallo ? -fallo : 0;
}
#ifdef NXK_TEST
/* La instruccion siguiente a INT81 sale con el EAX recibido del kernel.
 * Una cantidad invalida se rechaza sin bloquear y devuelve UINT32_MAX. */
static int probar_intervalo(uint32_t ticks) {
    unsigned char codigo[]={0xbb,0,0,0,0,0xb8,10,0,0,0,0xcd,0x81,0xcd,0x80};
    for (unsigned i=0;i<4;++i) codigo[1+i]=(unsigned char)(ticks>>(8*i));
    uint32_t id=0, libres=nxk_paginas_libres(); int fallo=0;
    struct nxk_proceso_info p;
    if (nxk_proceso_crear(codigo,sizeof(codigo),&id)) return 1;
    if (nxk_proceso_paso(id) || nxk_proceso_consultar(id,&p)) fallo=2;
    if (!fallo && !ticks) {
        if (p.estado!=NXK_PROCESO_LISTO || p.turnos!=1 || p.cesiones!=1 ||
            p.dormidas || p.despertar || p.ultimo.fallo || p.error_motor ||
            nxk_proceso_paso(id) || nxk_proceso_consultar(id,&p) ||
            p.estado!=NXK_PROCESO_TERMINADO || p.turnos!=2 ||
            p.ultimo.fallo || p.ultimo.salida || p.dormidas) fallo=3;
    } else if (!fallo && ticks==0x7fffffffu) {
        if (p.estado!=NXK_PROCESO_DORMIDO || p.turnos!=1 || p.dormidas!=1 ||
            p.cesiones || p.preempciones || p.ultimo.fallo || p.error_motor ||
            nxk_proceso_paso(id)!=-2 || nxk_procesos_ejecutar(8)!=2) fallo=4;
    } else if (!fallo) {
        if (p.estado!=NXK_PROCESO_TERMINADO || p.turnos!=1 || p.dormidas ||
            p.cesiones || p.despertar || p.ultimo.fallo || p.error_motor ||
            p.ultimo.salida!=UINT32_MAX) fallo=5;
    }
    if (limpiar(&id,1)) fallo=6;
    /* No dejar que una espera vieja despierte un slot reciclado. */
    uint32_t viejo=id; memset(&p,0xa5,sizeof(p));
    if (nxk_proceso_consultar(viejo,&p)!=-1 || nxk_proceso_paso(viejo)!=-1 ||
        nxk_proceso_gestionado(viejo)) fallo=7;
    for (unsigned i=0;i<sizeof(p);++i) if (((unsigned char *)&p)[i]!=0xa5) fallo=7;
    if (nxk_proceso_crear(vecino,sizeof(vecino),&id)) fallo=8;
    else {
        if (id==viejo || nxk_proceso_consultar(id,&p) || p.estado!=NXK_PROCESO_LISTO ||
            p.turnos || p.dormidas || p.despertar || nxk_proceso_paso(id) ||
            nxk_proceso_consultar(id,&p) || p.estado!=NXK_PROCESO_TERMINADO ||
            p.ultimo.fallo || p.ultimo.salida!=7) fallo=8;
        if (limpiar(&id,1)) fallo=9;
    }
    if (nxk_paginas_libres()!=libres || cr3_actual()!=0x200000u) fallo=10;
    return fallo;
}
int nxk_proceso_dormir_probar(void) {
    if (!contexto_demo()) return 1;
    int r=nxk_proceso_dormir_demo(); if (r) return 10-r;
    size_t heap=nxk_heap_used(); uint32_t libres=nxk_paginas_libres(),guardas=nxk_paginacion_guardas();
    const uint32_t intervalos[]={0,0x7fffffffu,0x80000000u,UINT32_MAX};
    for (unsigned i=0;i<4;++i) { r=probar_intervalo(intervalos[i]); if (r) return 30+i*10+(unsigned)r; }
    if (nxk_proceso_dormir_demo() || nxk_proceso_dormir_demo()) return 80;
    return nxk_heap_used()!=heap || nxk_paginas_libres()!=libres ||
        nxk_paginacion_guardas()!=guardas || cr3_actual()!=0x200000u ? 90 : 0;
}
#endif
#else
int nxk_proceso_dormir_demo(void) { return -64; }
#endif
