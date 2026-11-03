/* Puente C: planificador cooperativo reutilizable. Cambio de pila NEXTRY ASM.
 * No es seguro para callbacks no confiables: todos ejecutan en ring0. */
#include "tareas.h"
#include "paginacion.h"
#if defined(__i386__)
extern void nxk_cambiar_tarea(uint32_t **anterior,uint32_t *siguiente);
extern uint32_t nxk_reloj_ticks(void);
struct tarea {
    uint32_t generacion, id, estado, activaciones, despertar;
    uint32_t resultado, espera_fin;
    uint32_t *pila;
    struct nxk_mensaje buzon[NXK_BUZON_MAX];
    unsigned lectura, cantidad;
    unsigned char *reserva, *memoria;
    void (*funcion)(void *);
    void *argumento;
};
static struct tarea tareas[NXK_TAREAS_MAX];
static uint32_t *principal;
static unsigned actual=NXK_TAREAS_MAX, cursor;
static int organizando;
#define SELLO_PILA 0x4e584b50u
static int interrupciones(void) {
    uint32_t f; __asm__ volatile("pushfl; popl %0":"=r"(f)); return (f&0x200)!=0;
}
/* Intervalos limitados a menos de media vuelta de uint32_t. */
static int vencido(uint32_t ahora,uint32_t plazo) { return ahora-plazo<0x80000000u; }
static struct tarea *buscar(uint32_t id) {
    unsigned i=(id&255u)-1u;
    if (i>=NXK_TAREAS_MAX || tareas[i].estado==NXK_TAREA_LIBRE || tareas[i].id!=id) return 0;
    return &tareas[i];
}
/* Publicar resultado antes de despertar a observadores. Sus referencias
 * se mantienen hasta reanudar o cancelar, para impedir reutilizar el destino. */
static void terminar(struct tarea *t,uint32_t estado,uint32_t codigo) {
    t->resultado=codigo; t->espera_fin=0; t->estado=estado;
    for (unsigned i=0;i<NXK_TAREAS_MAX;++i)
        if (tareas[i].estado==NXK_TAREA_ESPERANDO_FIN && tareas[i].espera_fin==t->id)
            tareas[i].estado=NXK_TAREA_LISTA;
}
static int pila_valida(struct tarea *t) {
    uintptr_t p=(uintptr_t)t->pila, base=(uintptr_t)t->memoria;
    return t->memoria && *(uint32_t *)t->memoria==SELLO_PILA && !(p&3) &&
        p>=base+64 && p<=base+4096-20 && nxk_paginacion_es_guarda(t->memoria-4096);
}
/* La funcion retorna a este envoltorio: nunca cae fuera de su pila. */
static _Noreturn void entrada(void) {
    struct tarea *t=&tareas[actual];
    t->funcion(t->argumento);
    nxk_tarea_finalizar(0);
    for (;;) __asm__ volatile("ud2");
}
int nxk_tarea_crear(void (*funcion)(void *),void *argumento,uint32_t *id) {
    if (!funcion || !id) return -1;
    unsigned i;
    /* Agotar generaciones retira el slot: nunca reaparece un ID antiguo. */
    for (i=0;i<NXK_TAREAS_MAX;++i)
        if (tareas[i].estado==NXK_TAREA_LIBRE && tareas[i].generacion<0xffffffu) break;
    if (i==NXK_TAREAS_MAX) return -2;
    struct tarea *t=&tareas[i];
    if (!t->reserva) { t->reserva=nxk_alloc(3*4096); if (!t->reserva) return -3; }
    uintptr_t guarda=((uintptr_t)t->reserva+4095)&~(uintptr_t)4095;
    if (nxk_paginacion_guarda((void *)guarda)) return -4;
    t->memoria=(unsigned char *)(guarda+4096);
    /* Limpieza al reutilizar, sin tocar la pagina ausente de guarda. */
    memset(t->memoria,0,4096);
    uint32_t *p=(uint32_t *)(t->memoria+4096-4);
    *--p=(uint32_t)(uintptr_t)entrada;
    for (unsigned r=0;r<4;++r) *--p=0;
    *(uint32_t *)t->memoria=SELLO_PILA; t->pila=p;
    ++t->generacion; t->id=(t->generacion<<8)|(i+1);
    t->funcion=funcion; t->argumento=argumento; t->activaciones=0; t->despertar=0;
    t->resultado=t->espera_fin=0;
    t->lectura=t->cantidad=0; memset(t->buzon,0,sizeof(t->buzon));
    t->estado=NXK_TAREA_LISTA; *id=t->id; return 0;
}
int nxk_tarea_consultar(uint32_t id,struct nxk_tarea_info *info) {
    struct tarea *t=buscar(id); if (!t || !info) return -1;
    info->id=t->id; info->estado=t->estado; info->activaciones=t->activaciones;
    info->despertar=t->despertar; info->resultado=t->resultado;
    info->esperando=t->espera_fin; return 0;
}
int nxk_tarea_enumerar(unsigned slot,struct nxk_tarea_info *info) {
    if (slot>=NXK_TAREAS_MAX || !info) return -1;
    if (tareas[slot].estado==NXK_TAREA_LIBRE) return -2;
    return nxk_tarea_consultar(tareas[slot].id,info);
}
uint32_t nxk_tarea_actual(void) { return actual<NXK_TAREAS_MAX ? tareas[actual].id : 0; }
int nxk_tarea_ceder(void) {
    if (actual>=NXK_TAREAS_MAX || !interrupciones()) return -1;
    struct tarea *t=&tareas[actual]; t->estado=NXK_TAREA_LISTA;
    nxk_cambiar_tarea(&t->pila,principal); return 0;
}
int nxk_tarea_dormir(uint32_t ticks) {
    if (ticks>0x7fffffffu || actual>=NXK_TAREAS_MAX || !interrupciones()) return -1;
    if (!ticks) return nxk_tarea_ceder();
    struct tarea *t=&tareas[actual];
    t->despertar=nxk_reloj_ticks()+ticks; t->estado=NXK_TAREA_DORMIDA;
    nxk_cambiar_tarea(&t->pila,principal); return 0;
}
/* Una CPU, sin ceder dentro de la operacion, ni acceso a buzones desde IRQ.
 * Copiar el mensaje antes de publicar cantidad conserva el orden FIFO. */
int nxk_tarea_enviar(uint32_t destino,uint32_t valor) {
    struct tarea *t=buscar(destino); if (!t) return -1;
    if (t->estado==NXK_TAREA_TERMINADA || t->estado==NXK_TAREA_CANCELADA) return -3;
    if (t->cantidad==NXK_BUZON_MAX) return -2;
    unsigned i=(t->lectura+t->cantidad)%NXK_BUZON_MAX;
    t->buzon[i].origen=nxk_tarea_actual(); t->buzon[i].valor=valor;
    ++t->cantidad;
    /* Solo la espera de mensajes se despierta: dormir conserva su plazo. */
    if (t->estado==NXK_TAREA_ESPERANDO) t->estado=NXK_TAREA_LISTA;
    return 0;
}
int nxk_tarea_recibir(struct nxk_mensaje *mensaje) {
    if (!mensaje || actual>=NXK_TAREAS_MAX) return -1;
    struct tarea *t=&tareas[actual]; if (!t->cantidad) return -2;
    *mensaje=t->buzon[t->lectura]; memset(&t->buzon[t->lectura],0,sizeof(*mensaje));
    t->lectura=(t->lectura+1)%NXK_BUZON_MAX; --t->cantidad; return 0;
}
int nxk_tarea_esperar_mensaje(struct nxk_mensaje *mensaje,uint32_t ticks) {
    if (!mensaje || ticks>0x7fffffffu || actual>=NXK_TAREAS_MAX || !interrupciones()) return -1;
    if (!nxk_tarea_recibir(mensaje)) return 0;
    if (!ticks) return 1;
    struct tarea *t=&tareas[actual];
    t->despertar=nxk_reloj_ticks()+ticks; t->estado=NXK_TAREA_ESPERANDO;
    /* Sin otra tarea entre comprobar el buzon y publicar la espera. */
    nxk_cambiar_tarea(&t->pila,principal);
    /* Un mensaje disponible al reanudar tiene prioridad sobre el vencimiento. */
    return nxk_tarea_recibir(mensaje)==0 ? 0 : 1;
}
int nxk_tarea_finalizar(uint32_t codigo) {
    if (actual>=NXK_TAREAS_MAX || !interrupciones()) return -1;
    struct tarea *t=&tareas[actual]; terminar(t,NXK_TAREA_TERMINADA,codigo);
    nxk_cambiar_tarea(&t->pila,principal);
    for (;;) __asm__ volatile("ud2");
}
int nxk_tarea_esperar_fin(uint32_t id,struct nxk_tarea_resultado *salida,uint32_t ticks) {
    if (!salida || ticks>0x7fffffffu || actual>=NXK_TAREAS_MAX || !interrupciones()) return -1;
    struct tarea *destino=buscar(id), *t=&tareas[actual]; if (!destino) return -1;
    /* Revisar la cadena bloqueada evita autoespera y ciclos entre tareas. */
    struct tarea *p=destino;
    for (unsigned n=0;n<NXK_TAREAS_MAX;++n) {
        if (p==t) return -2;
        if (p->estado!=NXK_TAREA_ESPERANDO_FIN) break;
        p=buscar(p->espera_fin); if (!p) return -1;
        if (n+1==NXK_TAREAS_MAX) return -2;
    }
    if (destino->estado!=NXK_TAREA_TERMINADA && destino->estado!=NXK_TAREA_CANCELADA) {
        if (!ticks) return 1;
        t->espera_fin=id; t->despertar=nxk_reloj_ticks()+ticks; t->estado=NXK_TAREA_ESPERANDO_FIN;
        nxk_cambiar_tarea(&t->pila,principal);
        /* liberar() protege la referencia incluso despues de ser despertado. */
        destino=buscar(id); t->espera_fin=0;
        if (!destino) return -1;
        if (destino->estado!=NXK_TAREA_TERMINADA && destino->estado!=NXK_TAREA_CANCELADA) return 1;
    }
    salida->estado=destino->estado; salida->codigo=destino->resultado; return 0;
}
int nxk_tarea_cancelar(uint32_t id) {
    struct tarea *t=buscar(id); if (!t) return -1;
    if (t->estado!=NXK_TAREA_LISTA && t->estado!=NXK_TAREA_DORMIDA &&
        t->estado!=NXK_TAREA_ESPERANDO && t->estado!=NXK_TAREA_ESPERANDO_FIN) return -2;
    terminar(t,NXK_TAREA_CANCELADA,UINT32_MAX); return 0;
}
int nxk_tarea_liberar(uint32_t id) {
    struct tarea *t=buscar(id); if (!t) return -1;
    if (t->estado!=NXK_TAREA_TERMINADA && t->estado!=NXK_TAREA_CANCELADA) return -2;
    for (unsigned i=0;i<NXK_TAREAS_MAX;++i) if (tareas[i].espera_fin==id) return -3;
    t->funcion=0; t->argumento=0; t->cantidad=t->lectura=0;
    memset(t->buzon,0,sizeof(t->buzon)); t->estado=NXK_TAREA_LIBRE; return 0;
}
int nxk_tareas_ejecutar(uint32_t presupuesto) {
    if (organizando || !presupuesto || presupuesto>0x7fffffffu) return -1;
    if (!interrupciones()) return -4;
    organizando=1; uint32_t inicio=nxk_reloj_ticks(); int resultado=0;
    for (;;) {
        unsigned pendientes=0, elegida=NXK_TAREAS_MAX;
        uint32_t ahora=nxk_reloj_ticks();
        /* Buscar a partir del siguiente slot evita favorecer siempre al primero. */
        for (unsigned n=0;n<NXK_TAREAS_MAX;++n) {
            unsigned i=(cursor+n)%NXK_TAREAS_MAX; struct tarea *t=&tareas[i];
            if ((t->estado==NXK_TAREA_DORMIDA || t->estado==NXK_TAREA_ESPERANDO || t->estado==NXK_TAREA_ESPERANDO_FIN) && vencido(ahora,t->despertar)) t->estado=NXK_TAREA_LISTA;
            if (t->estado==NXK_TAREA_LISTA || t->estado==NXK_TAREA_DORMIDA ||
                t->estado==NXK_TAREA_ESPERANDO || t->estado==NXK_TAREA_ESPERANDO_FIN) ++pendientes;
            if (elegida==NXK_TAREAS_MAX && t->estado==NXK_TAREA_LISTA) elegida=i;
        }
        if (!pendientes) break;
        if ((uint32_t)(ahora-inicio)>=presupuesto) { resultado=1; break; }
        if (elegida==NXK_TAREAS_MAX) { __asm__ volatile("hlt"); continue; }
        struct tarea *t=&tareas[elegida];
        if (!pila_valida(t)) { terminar(t,NXK_TAREA_CANCELADA,UINT32_MAX); resultado=-5; break; }
        cursor=(elegida+1)%NXK_TAREAS_MAX; actual=elegida;
        t->estado=NXK_TAREA_EJECUTANDO; ++t->activaciones;
        nxk_cambiar_tarea(&principal,t->pila);
        actual=NXK_TAREAS_MAX;
        if (!pila_valida(t)) { terminar(t,NXK_TAREA_CANCELADA,UINT32_MAX); resultado=-5; break; }
        if (!interrupciones()) { resultado=-4; break; }
    }
    actual=NXK_TAREAS_MAX; organizando=0; return resultado;
}
#ifdef NXK_TEST
/* El organizador debe rechazar una pila guardada invalida antes de entrar. */
int nxk_planificador_probar_rechazo(uint32_t id) {
    struct tarea *t=buscar(id); if (!t || t->estado!=NXK_TAREA_LISTA) return 1;
    t->pila=(uint32_t *)(t->memoria+32);
    return nxk_tareas_ejecutar(10)!=-5 || t->estado!=NXK_TAREA_CANCELADA;
}
/* Comprueba limites del sello y comparacion modular sin provocar #PF. */
int nxk_planificador_probar_pila(uint32_t id) {
    struct tarea *t=buscar(id); if (!t || !pila_valida(t)) return 1;
    uint32_t *p=t->pila; t->pila=(uint32_t *)(t->memoria+32);
    int aceptada=pila_valida(t); t->pila=p; if (aceptada) return 2;
    *(uint32_t *)t->memoria=0; aceptada=pila_valida(t);
    *(uint32_t *)t->memoria=SELLO_PILA;
    return aceptada || !vencido(3,0xfffffffeu) || vencido(0xfffffffdu,3) || !vencido(7,7);
}
#endif
#else
/* x64 conserva arranque; esta API requiere el cambio de contexto x86. */
int nxk_tarea_crear(void (*f)(void *),void *a,uint32_t *id) { (void)f;(void)a;(void)id;return -64; }
int nxk_tarea_consultar(uint32_t id,struct nxk_tarea_info *i) { (void)id;(void)i;return -64; }
int nxk_tarea_enumerar(unsigned s,struct nxk_tarea_info *i) { (void)s;(void)i;return -64; }
uint32_t nxk_tarea_actual(void) { return 0; }
int nxk_tarea_ceder(void) { return -64; }
int nxk_tarea_dormir(uint32_t n) { (void)n;return -64; }
int nxk_tarea_enviar(uint32_t id,uint32_t v) { (void)id;(void)v;return -64; }
int nxk_tarea_recibir(struct nxk_mensaje *m) { (void)m;return -64; }
int nxk_tarea_esperar_mensaje(struct nxk_mensaje *m,uint32_t n) { (void)m;(void)n;return -64; }
int nxk_tarea_finalizar(uint32_t c) { (void)c;return -64; }
int nxk_tarea_esperar_fin(uint32_t id,struct nxk_tarea_resultado *s,uint32_t n) { (void)id;(void)s;(void)n;return -64; }
int nxk_tarea_cancelar(uint32_t id) { (void)id;return -64; }
int nxk_tarea_liberar(uint32_t id) { (void)id;return -64; }
int nxk_tareas_ejecutar(uint32_t n) { (void)n;return -64; }
#endif
