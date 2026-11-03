/* Demostraciones y pruebas del planificador. El cambio de contexto es NXA;
 * la organizacion, argumentos y verificaciones siguen como puente C. */
#include "tareas.h"
#include "paginacion.h"
#if defined(__i386__)
extern uint32_t nxk_reloj_ticks(void);
struct trabajo { uint32_t id, indice, pasos, fallos; };
static void trabajar(void *argumento) {
    struct trabajo *t=argumento;
    volatile uint32_t local=0x12340000u+t->indice;
    for (unsigned i=0;i<4;++i) {
        struct nxk_tarea_info info;
        if (local!=0x12340000u+t->indice || nxk_tarea_actual()!=t->id ||
            nxk_tarea_consultar(t->id,&info) || info.estado!=NXK_TAREA_EJECUTANDO) ++t->fallos;
        ++t->pasos; uint32_t antes=nxk_reloj_ticks();
        uint32_t espera=1+t->indice%2;
        if (nxk_tarea_dormir(espera) || (uint32_t)(nxk_reloj_ticks()-antes)<espera) ++t->fallos;
    }
}
/* Los argumentos sobreviven entre comandos; no apuntan a la pila de consola. */
static struct trabajo lote[NXK_TAREAS_MAX];
int nxk_tareas_limpiar_demo(void) {
    int fallo=0;
    for (unsigned i=0;i<NXK_TAREAS_MAX;++i) if (lote[i].id) {
        struct nxk_tarea_info info;
        if (nxk_tarea_consultar(lote[i].id,&info)) { lote[i].id=0; continue; }
        nxk_tarea_cancelar(lote[i].id);
        if (nxk_tarea_liberar(lote[i].id)) fallo=-1;
        else lote[i].id=0;
    }
    return fallo;
}
int nxk_tareas_preparar_demo(void) {
    for (unsigned i=0;i<NXK_TAREAS_MAX;++i) if (lote[i].id) return -1;
    memset(lote,0,sizeof(lote));
    for (unsigned i=0;i<NXK_TAREAS_MAX;++i) {
        lote[i].indice=i;
        if (nxk_tarea_crear(trabajar,&lote[i],&lote[i].id)) { nxk_tareas_limpiar_demo(); return -2; }
    }
    return 0;
}
int nxk_tareas_demo_cantidad(unsigned cantidad) {
    if (!cantidad || cantidad>NXK_TAREAS_MAX) return -1;
    struct trabajo trabajos[NXK_TAREAS_MAX]; memset(trabajos,0,sizeof(trabajos));
    unsigned creadas=0; int resultado=0;
    for (;creadas<cantidad;++creadas) {
        trabajos[creadas].indice=creadas;
        if (nxk_tarea_crear(trabajar,&trabajos[creadas],&trabajos[creadas].id)) { resultado=-2; break; }
    }
    if (!resultado) resultado=nxk_tareas_ejecutar(100);
    for (unsigned i=0;i<creadas;++i) {
        struct nxk_tarea_info info;
        if (!resultado && (trabajos[i].pasos!=4 || trabajos[i].fallos ||
            nxk_tarea_consultar(trabajos[i].id,&info) || info.estado!=NXK_TAREA_TERMINADA ||
            info.activaciones!=5)) resultado=-3;
        /* No dejar punteros a argumentos de esta pila despues de retornar. */
        nxk_tarea_cancelar(trabajos[i].id); nxk_tarea_liberar(trabajos[i].id);
    }
    return resultado;
}
int nxk_tareas_demo(void) { return nxk_tareas_demo_cantidad(2); }
#ifdef NXK_TEST
extern int nxk_planificador_probar_pila(uint32_t id);
extern int nxk_planificador_probar_rechazo(uint32_t id);
static uint32_t orden[3*NXK_TAREAS_MAX], posicion, errores;
static void ronda(void *argumento) {
    struct trabajo *t=argumento; volatile uint32_t local=0xaabb0000u+t->indice;
    if (nxk_tareas_ejecutar(10)!=-1 || nxk_tarea_cancelar(t->id)!=-2 ||
        nxk_tarea_liberar(t->id)!=-2 || nxk_tarea_dormir(0x80000000u)!=-1) ++errores;
    for (unsigned i=0;i<3;++i) {
        if (posicion>=3*NXK_TAREAS_MAX || local!=0xaabb0000u+t->indice) { ++errores; return; }
        orden[posicion++]=t->indice; ++t->pasos;
        if (nxk_tarea_dormir(0)) ++errores;
    }
}
static void sumar(void *p) { ++*(uint32_t *)p; }
static void esperar(void *p) { ++*(uint32_t *)p; nxk_tarea_dormir(4); ++*(uint32_t *)p; }
static uint32_t hijo, contador_hijo;
static void crear_hijo(void *p) {
    (void)p; if (nxk_tarea_crear(sumar,&contador_hijo,&hijo)) ++errores;
}
struct cancelacion { uint32_t victima, resultado; };
static void cancelar_otra(void *p) {
    struct cancelacion *c=p; c->resultado=(uint32_t)nxk_tarea_cancelar(c->victima);
}
/* Buzones: FIFO lleno, vacio sin modificar salida y mensajes a uno mismo. */
static void probar_buzon(void *argumento) {
    uint32_t *recibidos=argumento; struct nxk_mensaje m;
    for (unsigned i=0;i<NXK_BUZON_MAX;++i) {
        if (nxk_tarea_recibir(&m) || m.origen || m.valor!=100+i) ++errores;
        ++*recibidos;
    }
    m.origen=99; m.valor=88;
    if (nxk_tarea_recibir(&m)!=-2 || m.origen!=99 || m.valor!=88 ||
        nxk_tarea_recibir(0)!=-1 || nxk_tarea_enviar(0,1)!=-1) ++errores;
    for (unsigned i=0;i<9;++i) {
        if (nxk_tarea_enviar(nxk_tarea_actual(),i) || nxk_tarea_recibir(&m) ||
            m.origen!=nxk_tarea_actual() || m.valor!=i) ++errores;
    }
}
struct intercambio { uint32_t productor, consumidor, enviados, recibidos; };
static void producir(void *argumento) {
    struct intercambio *c=argumento;
    for (unsigned i=0;i<32;++i) {
        int r;
        while ((r=nxk_tarea_enviar(c->consumidor,i+500))==-2) nxk_tarea_ceder();
        if (r) { ++errores; return; }
        ++c->enviados;
    }
}
static void consumir(void *argumento) {
    struct intercambio *c=argumento;
    for (unsigned i=0;i<32;++i) {
        struct nxk_mensaje m; int r;
        while ((r=nxk_tarea_recibir(&m))==-2) nxk_tarea_ceder();
        if (r || m.origen!=c->productor || m.valor!=i+500) { ++errores; return; }
        ++c->recibidos;
    }
}
static void comprobar_vacio(void *argumento) {
    (void)argumento; struct nxk_mensaje m;
    if (nxk_tarea_recibir(&m)!=-2) ++errores;
}
int nxk_tareas_probar_limites(void) {
    struct nxk_tarea_info info; uint32_t salida=0xdeadbeefu;
    if (nxk_tarea_enumerar(NXK_TAREAS_MAX,&info)!=-1 || nxk_tarea_enumerar(0,0)!=-1 ||
        nxk_tarea_enumerar(0,&info)!=-2 || nxk_tarea_actual() || nxk_tarea_ceder()!=-1 || nxk_tarea_dormir(1)!=-1 ||
        nxk_tarea_consultar(0,&info)!=-1 || nxk_tarea_cancelar(0)!=-1 ||
        nxk_tarea_liberar(0)!=-1 || nxk_tareas_ejecutar(0)!=-1 ||
        nxk_tareas_ejecutar(0x80000000u)!=-1 || nxk_tarea_crear(0,0,&salida)!=-1 ||
        salida!=0xdeadbeefu || nxk_tarea_crear(sumar,&salida,0)!=-1) return 1;
    struct trabajo trabajos[NXK_TAREAS_MAX]; memset(trabajos,0,sizeof(trabajos));
    posicion=errores=0;
    for (unsigned i=0;i<NXK_TAREAS_MAX;++i) {
        trabajos[i].indice=i;
        if (nxk_tarea_crear(ronda,&trabajos[i],&trabajos[i].id) ||
            nxk_tarea_consultar(trabajos[i].id,&info) || info.estado!=NXK_TAREA_LISTA ||
            info.activaciones || nxk_tarea_liberar(trabajos[i].id)!=-2 ||
            nxk_planificador_probar_pila(trabajos[i].id)) return 2;
    }
    if (nxk_tarea_crear(sumar,&salida,&salida)!=-2 || salida!=0xdeadbeefu) return 3;
    uint32_t guardas=nxk_paginacion_guardas(); size_t usado=nxk_heap_used();
    uint32_t f; __asm__ volatile("pushfl; popl %0; cli":"=r"(f)::"memory");
    int sin_irq=nxk_tareas_ejecutar(20);
    __asm__ volatile("pushl %0; popfl"::"r"(f):"memory","cc");
    if (sin_irq!=-4 || nxk_tareas_ejecutar(100) || errores || posicion!=24) return 4;
    /* Las tres vueltas deben conservar el mismo orden circular, sin hambre. */
    unsigned vistos=0;
    for (unsigned i=0;i<NXK_TAREAS_MAX;++i) {
        if (orden[i]>=NXK_TAREAS_MAX || (vistos&(1u<<orden[i])) ||
            orden[i]!=orden[i+8] || orden[i]!=orden[i+16]) return 5;
        vistos|=1u<<orden[i];
        if (trabajos[i].pasos!=3 || nxk_tarea_consultar(trabajos[i].id,&info) ||
            info.estado!=NXK_TAREA_TERMINADA || info.activaciones!=4 ||
            nxk_tarea_cancelar(trabajos[i].id)!=-2 || nxk_tarea_liberar(trabajos[i].id)) return 6;
    }
    uint32_t antiguo=trabajos[0].id, nuevo, contador=0;
    if (nxk_tarea_crear(sumar,&contador,&nuevo) || antiguo==nuevo ||
        nxk_tarea_consultar(antiguo,&info)!=-1 || nxk_tarea_cancelar(antiguo)!=-1 ||
        nxk_tarea_liberar(antiguo)!=-1 || nxk_tarea_cancelar(nuevo) ||
        nxk_tarea_consultar(nuevo,&info) || info.estado!=NXK_TAREA_CANCELADA ||
        nxk_tareas_ejecutar(10) || contador || nxk_tarea_liberar(nuevo) ||
        nxk_tarea_liberar(nuevo)!=-1) return 7;
    if (nxk_tarea_crear(esperar,&contador,&nuevo) || nxk_tareas_ejecutar(1)!=1 ||
        contador!=1 || nxk_tarea_consultar(nuevo,&info) || info.estado!=NXK_TAREA_DORMIDA ||
        nxk_tarea_liberar(nuevo)!=-2 || nxk_tareas_ejecutar(30) || contador!=2 ||
        nxk_tarea_liberar(nuevo)) return 8;
    contador=0;
    if (nxk_tarea_crear(esperar,&contador,&nuevo) || nxk_tareas_ejecutar(1)!=1 ||
        nxk_tarea_cancelar(nuevo) || nxk_tareas_ejecutar(10) || contador!=1 ||
        nxk_tarea_liberar(nuevo)) return 9;
    contador_hijo=0; hijo=0;
    uint32_t padre;
    if (nxk_tarea_crear(crear_hijo,0,&padre) || nxk_tareas_ejecutar(100) || errores ||
        !hijo || contador_hijo!=1 || nxk_tarea_liberar(padre) || nxk_tarea_liberar(hijo)) return 10;
    struct cancelacion c; c.resultado=99; contador=0;
    if (nxk_tarea_crear(esperar,&contador,&c.victima) ||
        nxk_tarea_crear(cancelar_otra,&c,&nuevo) || nxk_tareas_ejecutar(30) ||
        c.resultado || contador>1 || nxk_tarea_consultar(c.victima,&info) ||
        info.estado!=NXK_TAREA_CANCELADA || nxk_tarea_liberar(c.victima) || nxk_tarea_liberar(nuevo)) return 11;
    contador=0;
    if (nxk_tarea_crear(sumar,&contador,&nuevo) || nxk_planificador_probar_rechazo(nuevo) ||
        contador || nxk_tarea_liberar(nuevo) || nxk_tareas_ejecutar(10)) return 18;
    struct nxk_mensaje m;
    contador=0;
    if (nxk_tarea_recibir(&m)!=-1 || nxk_tarea_enviar(0,1)!=-1 ||
        nxk_tarea_crear(probar_buzon,&contador,&nuevo)) return 13;
    for (unsigned i=0;i<NXK_BUZON_MAX;++i) if (nxk_tarea_enviar(nuevo,100+i)) return 14;
    if (nxk_tarea_enviar(nuevo,999)!=-2 || nxk_tareas_ejecutar(100) ||
        errores || contador!=4 || nxk_tarea_enviar(nuevo,5)!=-3 ||
        nxk_tarea_liberar(nuevo) || nxk_tarea_enviar(nuevo,5)!=-1) return 15;
    antiguo=nuevo;
    if (nxk_tarea_crear(comprobar_vacio,0,&nuevo) || nxk_tarea_enviar(antiguo,5)!=-1 ||
        nxk_tarea_enviar(nuevo,7) || nxk_tarea_cancelar(nuevo) ||
        nxk_tarea_enviar(nuevo,8)!=-3 || nxk_tarea_liberar(nuevo) ||
        nxk_tarea_crear(comprobar_vacio,0,&nuevo) || nxk_tareas_ejecutar(100) ||
        errores || nxk_tarea_liberar(nuevo)) return 16;
    struct intercambio intercambio; memset(&intercambio,0,sizeof(intercambio));
    if (nxk_tarea_crear(producir,&intercambio,&intercambio.productor) ||
        nxk_tarea_crear(consumir,&intercambio,&intercambio.consumidor) ||
        nxk_tareas_ejecutar(100) || errores || intercambio.enviados!=32 ||
        intercambio.recibidos!=32 || nxk_tarea_liberar(intercambio.productor) ||
        nxk_tarea_liberar(intercambio.consumidor)) return 17;
    if (nxk_tareas_demo_cantidad(8) || nxk_tareas_demo_cantidad(8) ||
        nxk_heap_used()!=usado || nxk_paginacion_guardas()!=guardas ||
        nxk_tarea_actual() || nxk_tareas_ejecutar(10)) return 12;
    return 0;
}
#endif
#else
int nxk_tareas_preparar_demo(void) { return -64; }
int nxk_tareas_limpiar_demo(void) { return -64; }
int nxk_tareas_demo_cantidad(unsigned n) { (void)n;return -64; }
int nxk_tareas_demo(void) { return -64; }
#endif
