/* Espera del resultado de otro proceso y retencion de su ID. Puente C de
 * demostracion; ambos programas CPL3 son instrucciones x86 fijas y acotadas.
 * No consume procesos ajenos ni modifica archivos o dispositivos. */
#include "procesos.h"
#include "paginas.h"
#include "paginacion.h"
#include "tareas.h"
#if defined(__i386__)
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
/* Ceder una vez y despues finalizar con codigo42. No depende del observador. */
static const unsigned char trabajador[]={0xb8,9,0,0,0,0xcd,0x81,0xb8,42,0,0,0,0xcd,0x80};
static int limpiar(uint32_t ids[2]) {
    int r=nxk_espacio_restaurar();
    /* Primero cancelar al observador, incluso en un fallo intermedio: mientras
     * conserve FIN el trabajador no puede liberarse por ninguna de las API. */
    if (ids[1]) nxk_proceso_cancelar(ids[1]);
    if (ids[0]) nxk_proceso_cancelar(ids[0]);
    for (unsigned i=0;i<2;++i) if (ids[i] && nxk_proceso_liberar(ids[i])) r=-1;
    return r;
}
int nxk_dependencias_demo(void) {
    if (!contexto_demo()) return -1;
    for (unsigned i=0;i<NXK_ESPACIOS_MAX;++i) {
        struct nxk_proceso_info p; if (!nxk_proceso_enumerar(i,&p)) return -2;
    }
    /* EBX=ID, ECX=buffer8 en pila privada, EDX=intervalo maximo. Al continuar
     * comprobar EAX0, estado3 y codigo42 antes de salir42. Cualquier diferencia
     * ejecuta UD2 y se convierte en un fallo visible, nunca en exito aparente. */
    unsigned char observador[]={
        0xbb,0,0,0,0,0xb9,0xf0,0x1f,0x40,0,0xba,0xff,0xff,0xff,0x7f,
        0xb8,15,0,0,0,0xcd,0x81,0x85,0xc0,0x75,0x12,
        0x83,0x39,3,0x75,0x0d,0x83,0x79,4,42,0x75,7,
        0xb8,42,0,0,0,0xcd,0x80,0x0f,0x0b
    };
    uint32_t ids[2]={0},libres=nxk_paginas_libres(),guardas=nxk_paginacion_guardas();
    size_t heap=nxk_heap_used(); int fallo=0; struct nxk_proceso_info a,b;
    if (nxk_proceso_crear(trabajador,sizeof(trabajador),&ids[0])) fallo=3;
    if (!fallo) {
        for (unsigned i=0;i<4;++i) observador[1+i]=(unsigned char)(ids[0]>>(8*i));
        if (nxk_proceso_crear(observador,sizeof(observador),&ids[1])) fallo=4;
    }
    if (!fallo && nxk_proceso_paso(ids[1])) fallo=5;
    /* La primera entrada CPL3 puede reservar la pila fija del motor. Las
     * entradas siguientes y las consultas rechazadas no deben crecer el heap. */
    heap=nxk_heap_used();
    if (!fallo && (nxk_proceso_consultar(ids[1],&a) ||
        a.estado!=NXK_PROCESO_ESPERANDO_FIN || a.turnos!=1 || a.esperas_fin!=1 ||
        a.esperando!=ids[0] || a.ultimo.fallo || a.error_motor ||
        !nxk_proceso_retenido(ids[0]) || nxk_proceso_liberar(ids[0])!=-5 ||
        nxk_espacio_destruir(ids[0])!=-5 || nxk_paginas_libres()!=libres-10)) fallo=6;
    if (!fallo && (nxk_proceso_paso(ids[0]) || nxk_proceso_consultar(ids[0],&b) ||
        b.estado!=NXK_PROCESO_LISTO || b.turnos!=1 || b.cesiones!=1 ||
        b.ultimo.fallo || b.error_motor || nxk_proceso_paso(ids[1])!=-2)) fallo=7;
    if (!fallo && (nxk_proceso_paso(ids[0]) || nxk_proceso_consultar(ids[0],&b) ||
        b.estado!=NXK_PROCESO_TERMINADO || b.turnos!=2 || b.ultimo.salida!=42 ||
        b.ultimo.fallo || b.error_motor || nxk_proceso_consultar(ids[1],&a) ||
        a.estado!=NXK_PROCESO_LISTO || a.turnos!=1 || a.esperando!=ids[0] ||
        !nxk_proceso_retenido(ids[0]) || nxk_proceso_liberar(ids[0])!=-5 ||
        nxk_espacio_destruir(ids[0])!=-5)) fallo=8;
    if (!fallo && (nxk_proceso_paso(ids[1]) || nxk_proceso_consultar(ids[1],&a) ||
        a.estado!=NXK_PROCESO_TERMINADO || a.turnos!=2 || a.esperas_fin!=1 ||
        a.ultimo.fallo || a.ultimo.salida!=42 || a.error_motor || a.esperando ||
        a.despertar || nxk_proceso_retenido(ids[0]) || nxk_heap_used()!=heap)) fallo=9;
    if (limpiar(ids)) fallo=10;
    if (nxk_paginas_libres()!=libres || nxk_paginacion_guardas()!=guardas ||
        nxk_heap_used()!=heap || cr3_actual()!=0x200000u) fallo=11;
    return fallo ? -fallo : 0;
}
#ifdef NXK_TEST
int nxk_dependencias_probar(void) {
    int r=nxk_dependencias_demo(); if (r) return 10-r;
    size_t heap=nxk_heap_used();
    uint32_t libres=nxk_paginas_libres(),guardas=nxk_paginacion_guardas();
    for (unsigned i=0;i<2;++i) { r=nxk_dependencias_demo(); if (r) return 30-r; }
    return nxk_heap_used()!=heap || nxk_paginas_libres()!=libres ||
        nxk_paginacion_guardas()!=guardas || cr3_actual()!=0x200000u ? 50 : 0;
}
#endif
#else
int nxk_dependencias_demo(void) { return -64; }
#endif
