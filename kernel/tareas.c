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
        pilas[i]=p; pasos[i]=0; estados[i]=LISTA;
    }
    fallos=0;
    for (unsigned vuelta=0;vuelta<5;++vuelta)
        for (actual=0;actual<2;++actual) {
            if (estados[actual]==TERMINADA) continue;
            estados[actual]=EJECUTANDO;
            nxk_cambiar_tarea(&principal,pilas[actual]);
            if (estados[actual]==EJECUTANDO) estados[actual]=LISTA;
        }
    return fallos || pasos[0]!=4 || pasos[1]!=4 ||
        estados[0]!=TERMINADA || estados[1]!=TERMINADA;
}
#else
int nxk_tareas_demo(void) { return -1; }
#endif
