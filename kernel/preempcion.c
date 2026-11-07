/* Prueba de preempcion real CPL3: el proceso ocupado no llama a ceder.
 * Sus registros y pila deben sobrevivir a dos IRQ0 y una app vecina termina.
 * Puente C sobre el cambio de contexto NEXTRY ASM. Solo consola, una CPU. */
#include "procesos.h"
#include "paginas.h"
#include "paginacion.h"
#include "tareas.h"
#if defined(__i386__)
static uint32_t cr3_actual(void) {
    uint32_t n; __asm__ volatile("mov %%cr3,%0":"=r"(n)); return n;
}
static int contexto_demo(void) {
    uint32_t flags; __asm__ volatile("pushfl; popl %0":"=r"(flags));
    return cr3_actual()==0x200000u && (flags&0x200u) && !nxk_tarea_actual();
}
/* A inicializa registros, apila un sello y gira hasta que el kernel pone
 * uno en401000. Cuenta iteraciones en EBX/401004 sin modificar EAX. Al salir
 * verifica seis GPR, ESP, DF y FS/GS nulos antes de devolver42; UD2 denuncia
 * cualquier cambio. El wrapper IRQ normaliza segmentos/DF solo para el kernel. */
static const unsigned char ocupado[]={
        0x31,0xc0,0x8e,0xe0,0x8e,0xe8,0xb8,0xdf,0x9b,0x57,0x13,0xbb,
        0x00,0x00,0x00,0x00,0xb9,0xe0,0xac,0x68,0x24,0xba,0x40,0x30,
        0x20,0x10,0xbe,0x88,0x77,0x66,0x55,0xbf,0x99,0x88,0x77,0x66,
        0xbd,0x44,0x33,0x22,0x11,0x68,0x10,0x32,0x54,0x76,0xfd,0x43,
        0x89,0x1d,0x04,0x10,0x40,0x00,0x83,0x3d,0x00,0x10,0x40,0x00,
        0x00,0x74,0xf0,0x3d,0xdf,0x9b,0x57,0x13,0x0f,0x85,0x83,0x00,
        0x00,0x00,0x81,0xf9,0xe0,0xac,0x68,0x24,0x0f,0x85,0x77,0x00,
        0x00,0x00,0x81,0xfa,0x40,0x30,0x20,0x10,0x0f,0x85,0x6b,0x00,
        0x00,0x00,0x81,0xfe,0x88,0x77,0x66,0x55,0x0f,0x85,0x5f,0x00,
        0x00,0x00,0x81,0xff,0x99,0x88,0x77,0x66,0x0f,0x85,0x53,0x00,
        0x00,0x00,0x81,0xfd,0x44,0x33,0x22,0x11,0x0f,0x85,0x47,0x00,
        0x00,0x00,0x81,0xfc,0xfc,0x1f,0x40,0x00,0x0f,0x85,0x3b,0x00,
        0x00,0x00,0x5a,0x81,0xfa,0x10,0x32,0x54,0x76,0x0f,0x85,0x2e,
        0x00,0x00,0x00,0x9c,0x5a,0xf7,0xc2,0x00,0x04,0x00,0x00,0x0f,
        0x84,0x20,0x00,0x00,0x00,0x66,0x8c,0xe0,0x66,0x85,0xc0,0x0f,
        0x85,0x14,0x00,0x00,0x00,0x66,0x8c,0xe8,0x66,0x85,0xc0,0x0f,
        0x85,0x08,0x00,0x00,0x00,0xfc,0xb8,0x2a,0x00,0x00,0x00,0xcd,
        0x80,0x0f,0x0b
};
static const unsigned char vecino[]={0xb8,7,0,0,0,0xcd,0x80};
int nxk_preempcion_demo(void) {
    if (!contexto_demo()) return -1;
    /* No ejecutar ni avanzar procesos ajenos por medio del planificador global. */
    for (unsigned i=0;i<NXK_ESPACIOS_MAX;++i) {
        struct nxk_proceso_info p; if (!nxk_proceso_enumerar(i,&p)) return -2;
    }
    uint32_t ids[2]={0},libres=nxk_paginas_libres(),guardas=nxk_paginacion_guardas();
    uint32_t primero=0,segundo=0,parar=1; int fallo=0;
    struct nxk_proceso_info a,b;
    if (nxk_proceso_crear(ocupado,sizeof(ocupado),&ids[0]) ||
        nxk_proceso_crear(vecino,sizeof(vecino),&ids[1])) fallo=3;
    /* La ronda termina dos turnos aunque A no ceda: B debe devolver7. */
    if (!fallo && (nxk_procesos_ejecutar(2)!=1 ||
        nxk_proceso_consultar(ids[0],&a) || nxk_proceso_consultar(ids[1],&b) ||
        a.estado!=NXK_PROCESO_LISTO || a.turnos!=1 || a.preempciones!=1 ||
        a.cesiones || a.ultimo.fallo || a.error_motor ||
        b.estado!=NXK_PROCESO_TERMINADO || b.turnos!=1 || b.preempciones ||
        b.cesiones || b.ultimo.fallo || b.ultimo.salida!=7 || b.error_motor)) fallo=4;
    if (!fallo && (nxk_espacio_leer(ids[0],NXK_ESPACIO_PILA+4,&primero,sizeof(primero)) || !primero)) fallo=5;
    if (!fallo && (nxk_proceso_paso(ids[0]) || nxk_proceso_consultar(ids[0],&a) ||
        a.estado!=NXK_PROCESO_LISTO || a.turnos!=2 || a.preempciones!=2 || a.cesiones ||
        a.ultimo.fallo || a.error_motor ||
        nxk_espacio_leer(ids[0],NXK_ESPACIO_PILA+4,&segundo,sizeof(segundo)) || segundo<=primero)) fallo=6;
    /* Reanudar hasta terminar permite comprobar que no se reinicio la entrada:
     * el contador y el sello siguen en su lugar y los GPR contienen sus valores. */
    if (!fallo && (nxk_espacio_escribir(ids[0],NXK_ESPACIO_PILA,&parar,sizeof(parar)) ||
        nxk_proceso_paso(ids[0]) || nxk_proceso_consultar(ids[0],&a) ||
        a.estado!=NXK_PROCESO_TERMINADO || a.turnos!=3 || a.preempciones!=2 ||
        a.cesiones || a.ultimo.fallo || a.ultimo.salida!=42 || a.error_motor)) fallo=7;
    /* Recuperar el mapa aun si una comprobacion del motor fallo. */
    if (cr3_actual()!=0x200000u) fallo=8;
    if (nxk_espacio_restaurar()) fallo=8;
    for (unsigned i=0;i<2;++i) if (ids[i] && nxk_proceso_liberar(ids[i])) fallo=8;
    if (nxk_paginas_libres()!=libres || nxk_paginacion_guardas()!=guardas || cr3_actual()!=0x200000u) fallo=9;
    return fallo ? -fallo : 0;
}
#ifdef NXK_TEST
int nxk_preempcion_probar(void) {
    /* El primer turno valido puede reservar la pila kernel reutilizable. */
    int r=nxk_preempcion_demo(); if (r) return 100-r;
    size_t heap=nxk_heap_used(); uint32_t libres=nxk_paginas_libres(),guardas=nxk_paginacion_guardas();
    for (unsigned i=0;i<2;++i) { r=nxk_preempcion_demo(); if (r) return 200-r; }
    return nxk_heap_used()!=heap || nxk_paginas_libres()!=libres ||
        nxk_paginacion_guardas()!=guardas || cr3_actual()!=0x200000u ? 300 : 0;
}
#endif
#else
int nxk_preempcion_demo(void) { return -64; }
#endif
