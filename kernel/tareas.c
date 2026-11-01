/* Tareas cooperativas de demostracion: puente C y cambio NEXTRY ASM.
 * Dos pilas privadas del heap; solo ring0 y una CPU. No hay preempcion. */
#include "nxk.h"
#if defined(__i386__)
extern void nxk_cambiar_tarea(uint32_t **anterior, uint32_t *siguiente);
static uint32_t *principal, *pilas[2];
enum estado_tarea { LISTA, EJECUTANDO, TERMINADA };
static enum estado_tarea estados[2];
static unsigned actual, pasos[2], fallos;
static unsigned char *memoria[2];
#define SELLO_PILA 0x4e584b50u
/* Deteccion al cambiar de contexto; no reemplaza paginas de guarda. */
static int pila_valida(unsigned id) {
    uintptr_t p=(uintptr_t)pilas[id], base=(uintptr_t)memoria[id];
    return *(uint32_t *)memoria[id]==SELLO_PILA && !(p&3) &&
        p>=base+64 && p<=base+4096-20;
}
static void ceder(void) { nxk_cambiar_tarea(&pilas[actual],principal); }
static void ejecutar(void) {
    unsigned id=actual;
    volatile uint32_t local=0x12340000u+id;
    for (unsigned i=0;i<4;++i) {
        if (local!=0x12340000u+id || actual!=id) ++fallos;
        ++pasos[id];
        ceder();
    }
    estados[id]=TERMINADA;
    for (;;) ceder();
}
int nxk_tareas_demo(void) {
    /* Reservar una sola vez: repetir el comando no consume mas heap. */
    for (unsigned i=0;i<2;++i) {
        if (!memoria[i]) memoria[i]=nxk_alloc(4096);
        if (!memoria[i]) return -1;
        uint32_t *p=(uint32_t *)(memoria[i]+4096-4);
        *--p=(uint32_t)(uintptr_t)ejecutar;
        for (unsigned r=0;r<4;++r) *--p=0;
        *(uint32_t *)memoria[i]=SELLO_PILA;
        pilas[i]=p; pasos[i]=0; estados[i]=LISTA;
    }
    fallos=0;
    for (unsigned vuelta=0;vuelta<5;++vuelta)
        for (actual=0;actual<2;++actual) {
            if (estados[actual]==TERMINADA) continue;
            if (!pila_valida(actual)) return -2;
            estados[actual]=EJECUTANDO;
            nxk_cambiar_tarea(&principal,pilas[actual]);
            if (!pila_valida(actual)) return -2;
            if (estados[actual]==EJECUTANDO) estados[actual]=LISTA;
        }
    return fallos || pasos[0]!=4 || pasos[1]!=4 ||
        estados[0]!=TERMINADA || estados[1]!=TERMINADA;
}
#ifdef NXK_TEST
/* Corrupcion controlada: demostrar que el comprobador rechaza ambos casos. */
int nxk_tareas_probar_limites(void) {
    if (nxk_tareas_demo()) return 1;
    size_t usado=nxk_heap_used();
    if (nxk_tareas_demo() || nxk_heap_used()!=usado) return 2;
    uint32_t *guardada=pilas[0];
    pilas[0]=(uint32_t *)(memoria[0]+32);
    int aceptada=pila_valida(0);
    pilas[0]=guardada;
    if (aceptada) return 3;
    *(uint32_t *)memoria[0]=0;
    aceptada=pila_valida(0);
    *(uint32_t *)memoria[0]=SELLO_PILA;
    return aceptada ? 4 : 0;
}
#endif

#else
int nxk_tareas_demo(void) { return -1; }
#endif
