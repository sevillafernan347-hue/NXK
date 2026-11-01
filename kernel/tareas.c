/* Tareas cooperativas de demostracion: puente C y cambio NEXTRY ASM.
 * Dos pilas privadas con pagina de guarda en el heap; solo ring0 y una CPU. No hay preempcion. */
#include "nxk.h"
#include "paginacion.h"
#if defined(__i386__)
extern void nxk_cambiar_tarea(uint32_t **anterior, uint32_t *siguiente);
static uint32_t *principal, *pilas[2];
enum estado_tarea { LISTA, EJECUTANDO, DORMIDA, TERMINADA };
extern uint32_t nxk_reloj_ticks(void);
static uint32_t despertar[2];
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
/* Comparacion modular valida para intervalos menores de 2^31 ticks. */
static int vencido(uint32_t ahora,uint32_t plazo) {
    return (uint32_t)(ahora-plazo)<0x80000000u;
}
static void dormir(uint32_t cantidad) {
    despertar[actual]=nxk_reloj_ticks()+cantidad;
    estados[actual]=DORMIDA;
    ceder();
}
static void trabajo(void) {
    unsigned id=actual;
    volatile uint32_t local=0x12340000u+id;
    for (unsigned i=0;i<4;++i) {
        if (local!=0x12340000u+id || actual!=id) ++fallos;
        ++pasos[id];
        uint32_t antes=nxk_reloj_ticks();
        dormir(id+1);
        if ((uint32_t)(nxk_reloj_ticks()-antes)<id+1) ++fallos;
    }
}
/* Retornar de la funcion de trabajo termina la tarea, nunca salta a basura. */
static void ejecutar(void) {
    trabajo();
    estados[actual]=TERMINADA;
    for (;;) ceder();
}
int nxk_tareas_demo(void) {
    uint32_t banderas;
    __asm__ volatile("pushfl; popl %0" : "=r"(banderas));
    /* El PIT no puede despertar tareas con IF deshabilitada. */
    if (!(banderas&0x200)) return -4;
    /* Reservar una sola vez: repetir el comando no consume mas heap. */
    for (unsigned i=0;i<2;++i) {
        if (!memoria[i]) {
            /* Alineacion sin modificar el asignador: tres paginas reservadas
             * contienen una guarda alineada y una pila de 4 KiB tras ella. */
            unsigned char *reserva=nxk_alloc(3*4096);
            if (!reserva) return -1;
            uintptr_t guarda=((uintptr_t)reserva+4095)&~(uintptr_t)4095;
            if (nxk_paginacion_guarda((void *)guarda)) return -5;
            memoria[i]=(unsigned char *)(guarda+4096);
        }
        if (!nxk_paginacion_es_guarda(memoria[i]-4096)) return -5;
        uint32_t *p=(uint32_t *)(memoria[i]+4096-4);
        *--p=(uint32_t)(uintptr_t)ejecutar;
        for (unsigned r=0;r<4;++r) *--p=0;
        *(uint32_t *)memoria[i]=SELLO_PILA;
        pilas[i]=p; pasos[i]=0; estados[i]=LISTA;
    }
    fallos=0;
    uint32_t inicio=nxk_reloj_ticks();
    for (;;) {
        unsigned pendientes=0, ejecutadas=0;
        for (actual=0;actual<2;++actual) {
            if (estados[actual]==TERMINADA) continue;
            ++pendientes;
            if (estados[actual]==DORMIDA) {
                if (!vencido(nxk_reloj_ticks(),despertar[actual])) continue;
                estados[actual]=LISTA;
            }
            if (!pila_valida(actual)) return -2;
            estados[actual]=EJECUTANDO;
            nxk_cambiar_tarea(&principal,pilas[actual]);
            ++ejecutadas;
            if (!pila_valida(actual)) return -2;
            if (estados[actual]==EJECUTANDO) estados[actual]=LISTA;
        }
        if (!pendientes) break;
        /* Cota del organizador; no puede detener una tarea que nunca ceda. */
        if ((uint32_t)(nxk_reloj_ticks()-inicio)>100) return -3;
        if (!ejecutadas) __asm__ volatile("hlt");
    }
    return fallos || pasos[0]!=4 || pasos[1]!=4 ||
        estados[0]!=TERMINADA || estados[1]!=TERMINADA;
}
#ifdef NXK_TEST
/* Corrupcion controlada: demostrar que el comprobador rechaza ambos casos. */
int nxk_tareas_probar_limites(void) {
    if (!vencido(3,0xfffffffeu) || vencido(0xfffffffdu,3) || !vencido(7,7)) return 5;
    if (nxk_tareas_demo()) return 1;
    uint32_t guardas=nxk_paginacion_guardas();
    if (guardas!=2 || nxk_paginacion_guarda(memoria[0]-4096) ||
        nxk_paginacion_guardas()!=guardas) return 6;
    size_t usado=nxk_heap_used();
    if (nxk_tareas_demo() || nxk_heap_used()!=usado || nxk_paginacion_guardas()!=guardas) return 2;
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
