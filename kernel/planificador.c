/* Puente C: planificador cooperativo reutilizable. Cambio de pila NEXTRY ASM.
 * No es seguro para callbacks no confiables: todos ejecutan en ring0. */
#include "tareas.h"
#include "paginacion.h"
#include "semaforos.h"
#if defined(__i386__)
extern void nxk_cambiar_tarea(uint32_t **anterior,uint32_t *siguiente);
extern uint32_t nxk_reloj_ticks(void);
struct tarea {
    uint32_t generacion, id, estado, activaciones, despertar;
    uint32_t resultado, espera_fin, espera_envio, espera_semaforo;
    struct nxk_tarea_metricas metricas;
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
/* Saturar evita que un contador muy usado parezca volver a cero. */
static void incrementar(uint32_t *n) { if (*n!=UINT32_MAX) ++*n; }
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
/* Una dependencia representa una espera realmente bloqueada. Las tareas
 * ya listas por timeout/evento no prolongan una cadena aunque retengan un ID. */
static int crea_ciclo(struct tarea *origen,struct tarea *destino) {
    struct tarea *p=destino;
    for (unsigned n=0;n<NXK_TAREAS_MAX && p;++n) {
        if (p==origen) return 1;
        uint32_t id;
        if (p->estado==NXK_TAREA_ESPERANDO_FIN) id=p->espera_fin;
        else if (p->estado==NXK_TAREA_ESPERANDO_ENVIO) id=p->espera_envio;
        else return 0;
        p=buscar(id);
    }
    /* Una referencia a una generacion ya liberada se resolvera como error;
     * no forma una dependencia sobre el slot nuevo. */
    return p!=0;
}
/* Publicar resultado antes de despertar a observadores. Sus referencias
 * se mantienen hasta reanudar o cancelar, para impedir reutilizar el destino. */
static void terminar(struct tarea *t,uint32_t estado,uint32_t codigo) {
    t->resultado=codigo; t->espera_fin=t->espera_envio=t->espera_semaforo=0; t->estado=estado;
    for (unsigned i=0;i<NXK_TAREAS_MAX;++i)
        if ((tareas[i].estado==NXK_TAREA_ESPERANDO_FIN && tareas[i].espera_fin==t->id) ||
            (tareas[i].estado==NXK_TAREA_ESPERANDO_ENVIO && tareas[i].espera_envio==t->id))
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
    t->resultado=t->espera_fin=t->espera_envio=t->espera_semaforo=0;
    memset(&t->metricas,0,sizeof(t->metricas));
    t->lectura=t->cantidad=0; memset(t->buzon,0,sizeof(t->buzon));
    t->estado=NXK_TAREA_LISTA; *id=t->id; return 0;
}
int nxk_tarea_consultar(uint32_t id,struct nxk_tarea_info *info) {
    struct tarea *t=buscar(id); if (!t || !info) return -1;
    info->id=t->id; info->estado=t->estado; info->activaciones=t->activaciones;
    info->despertar=t->despertar; info->resultado=t->resultado;
    info->esperando=t->espera_fin ? t->espera_fin : t->espera_envio ? t->espera_envio : t->espera_semaforo; return 0;
}
int nxk_tarea_metricas(uint32_t id,struct nxk_tarea_metricas *salida) {
    struct tarea *t=buscar(id); if (!t || !salida) return -1;
    *salida=t->metricas; salida->activaciones=t->activaciones; return 0;
}
int nxk_planificador_consultar(struct nxk_planificador_resumen *salida) {
    if (!salida) return -1;
    struct nxk_planificador_resumen r; memset(&r,0,sizeof(r));
    for (unsigned i=0;i<NXK_TAREAS_MAX;++i) {
        struct tarea *t=&tareas[i];
        /* No convertir metadatos corruptos en un indice de escritura. */
        if (t->estado>=NXK_TAREA_ESTADOS || t->cantidad>NXK_BUZON_MAX) return -2;
        ++r.estados[t->estado];
        if (t->reserva) ++r.pilas_reservadas;
        r.mensajes+=t->cantidad;
        if (t->estado==NXK_TAREA_LIBRE) continue;
        ++r.registradas;
        if (t->estado==NXK_TAREA_TERMINADA || t->estado==NXK_TAREA_CANCELADA) ++r.finalizadas;
        else ++r.pendientes;
    }
    *salida=r; return 0;
}
int nxk_tarea_enumerar(unsigned slot,struct nxk_tarea_info *info) {
    if (slot>=NXK_TAREAS_MAX || !info) return -1;
    if (tareas[slot].estado==NXK_TAREA_LIBRE) return -2;
    return nxk_tarea_consultar(tareas[slot].id,info);
}
uint32_t nxk_tarea_actual(void) { return actual<NXK_TAREAS_MAX ? tareas[actual].id : 0; }
int nxk_tarea_ceder(void) {
    if (actual>=NXK_TAREAS_MAX || !interrupciones()) return -1;
    struct tarea *t=&tareas[actual]; incrementar(&t->metricas.cesiones); t->estado=NXK_TAREA_LISTA;
    nxk_cambiar_tarea(&t->pila,principal); return 0;
}
int nxk_tarea_dormir(uint32_t ticks) {
    if (ticks>0x7fffffffu || actual>=NXK_TAREAS_MAX || !interrupciones()) return -1;
    if (!ticks) return nxk_tarea_ceder();
    struct tarea *t=&tareas[actual];
    incrementar(&t->metricas.dormidas);
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
    if (actual<NXK_TAREAS_MAX) incrementar(&tareas[actual].metricas.enviados);
    /* Solo la espera de mensajes se despierta: dormir conserva su plazo. */
    if (t->estado==NXK_TAREA_ESPERANDO) t->estado=NXK_TAREA_LISTA;
    return 0;
}
int nxk_tarea_enviar_esperando(uint32_t destino,uint32_t valor,uint32_t ticks) {
    if (ticks>0x7fffffffu || actual>=NXK_TAREAS_MAX || !interrupciones()) return -1;
    struct tarea *t=&tareas[actual]; uint32_t inicio=nxk_reloj_ticks();
    for (;;) {
        /* Buscar por ID completo en cada intento: nunca usar un slot reciclado. */
        int r=nxk_tarea_enviar(destino,valor); if (r!=-2) return r;
        if (!ticks) return 1;
        if ((uint32_t)(nxk_reloj_ticks()-inicio)>=ticks) { incrementar(&t->metricas.plazos); return 1; }
        if (crea_ciclo(t,buscar(destino))) { incrementar(&t->metricas.ciclos); return -4; }
        incrementar(&t->metricas.esperas_envio);
        t->espera_envio=destino; t->despertar=inicio+ticks; t->estado=NXK_TAREA_ESPERANDO_ENVIO;
        nxk_cambiar_tarea(&t->pila,principal); t->espera_envio=0;
        /* Reintentar con el mismo plazo: competir no alarga indefinidamente la espera. */
    }
}
int nxk_tarea_recibir(struct nxk_mensaje *mensaje) {
    if (!mensaje || actual>=NXK_TAREAS_MAX) return -1;
    struct tarea *t=&tareas[actual]; if (!t->cantidad) return -2;
    *mensaje=t->buzon[t->lectura]; memset(&t->buzon[t->lectura],0,sizeof(*mensaje));
    t->lectura=(t->lectura+1)%NXK_BUZON_MAX; --t->cantidad;
    incrementar(&t->metricas.recibidos);
    /* Despertar emisores tras publicar el hueco. La ronda decide quien encola. */
    for (unsigned i=0;i<NXK_TAREAS_MAX;++i)
        if (tareas[i].estado==NXK_TAREA_ESPERANDO_ENVIO && tareas[i].espera_envio==t->id)
            tareas[i].estado=NXK_TAREA_LISTA;
    return 0;
}
int nxk_tarea_esperar_mensaje(struct nxk_mensaje *mensaje,uint32_t ticks) {
    if (!mensaje || ticks>0x7fffffffu || actual>=NXK_TAREAS_MAX || !interrupciones()) return -1;
    if (!nxk_tarea_recibir(mensaje)) return 0;
    if (!ticks) return 1;
    struct tarea *t=&tareas[actual];
    incrementar(&t->metricas.esperas_mensaje);
    t->despertar=nxk_reloj_ticks()+ticks; t->estado=NXK_TAREA_ESPERANDO;
    /* Sin otra tarea entre comprobar el buzon y publicar la espera. */
    nxk_cambiar_tarea(&t->pila,principal);
    /* Un mensaje disponible al reanudar tiene prioridad sobre el vencimiento. */
    if (!nxk_tarea_recibir(mensaje)) return 0;
    incrementar(&t->metricas.plazos); return 1;
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
    if (crea_ciclo(t,destino)) { incrementar(&t->metricas.ciclos); return -2; }
    if (destino->estado!=NXK_TAREA_TERMINADA && destino->estado!=NXK_TAREA_CANCELADA) {
        if (!ticks) return 1;
        incrementar(&t->metricas.esperas_fin);
        t->espera_fin=id; t->despertar=nxk_reloj_ticks()+ticks; t->estado=NXK_TAREA_ESPERANDO_FIN;
        nxk_cambiar_tarea(&t->pila,principal);
        /* liberar() protege la referencia incluso despues de ser despertado. */
        destino=buscar(id); t->espera_fin=0;
        if (!destino) return -1;
        if (destino->estado!=NXK_TAREA_TERMINADA && destino->estado!=NXK_TAREA_CANCELADA) { incrementar(&t->metricas.plazos); return 1; }
    }
    salida->estado=destino->estado; salida->codigo=destino->resultado; return 0;
}
int nxk_tarea_cancelar(uint32_t id) {
    struct tarea *t=buscar(id); if (!t) return -1;
    if (t->estado!=NXK_TAREA_LISTA && t->estado!=NXK_TAREA_DORMIDA &&
        t->estado!=NXK_TAREA_ESPERANDO && t->estado!=NXK_TAREA_ESPERANDO_FIN &&
        t->estado!=NXK_TAREA_ESPERANDO_ENVIO && t->estado!=NXK_TAREA_ESPERANDO_SEMAFORO) return -2;
    terminar(t,NXK_TAREA_CANCELADA,UINT32_MAX); return 0;
}
int nxk_tarea_liberar(uint32_t id) {
    struct tarea *t=buscar(id); if (!t) return -1;
    if (t->estado!=NXK_TAREA_TERMINADA && t->estado!=NXK_TAREA_CANCELADA) return -2;
    for (unsigned i=0;i<NXK_TAREAS_MAX;++i) if (tareas[i].espera_fin==id) return -3;
    t->funcion=0; t->argumento=0; t->cantidad=t->lectura=0;
    memset(t->buzon,0,sizeof(t->buzon)); t->estado=NXK_TAREA_LIBRE; return 0;
}
/* Objetos estaticos: no consumen heap al crear ni al esperar. */
struct semaforo { uint32_t generacion, id, valor, maximo, activo; };
static struct semaforo semaforos[NXK_SEMAFOROS_MAX];
static struct semaforo *buscar_semaforo(uint32_t id) {
    unsigned i=(id&255u)-1u;
    return i<NXK_SEMAFOROS_MAX && semaforos[i].activo && semaforos[i].id==id ? &semaforos[i] : 0;
}
static unsigned referencias_semaforo(uint32_t id) {
    unsigned n=0;
    for (unsigned i=0;i<NXK_TAREAS_MAX;++i) if (tareas[i].espera_semaforo==id) ++n;
    return n;
}
int nxk_semaforo_crear(uint32_t inicial,uint32_t maximo,uint32_t *id) {
    if (!id || !maximo || inicial>maximo) return -1;
    for (unsigned i=0;i<NXK_SEMAFOROS_MAX;++i) {
        struct semaforo *s=&semaforos[i];
        if (s->activo || s->generacion==0xffffffu) continue;
        ++s->generacion; s->id=(s->generacion<<8)|(i+1);
        s->valor=inicial; s->maximo=maximo; s->activo=1; *id=s->id; return 0;
    }
    return -2;
}
int nxk_semaforo_consultar(uint32_t id,struct nxk_semaforo_info *salida) {
    struct semaforo *s=buscar_semaforo(id); if (!s || !salida) return -1;
    salida->id=s->id; salida->disponibles=s->valor; salida->maximo=s->maximo;
    salida->esperadores=referencias_semaforo(id); return 0;
}
int nxk_semaforo_publicar(uint32_t id) {
    struct semaforo *s=buscar_semaforo(id); if (!s) return -1;
    if (s->valor==s->maximo) return -2;
    ++s->valor;
    for (unsigned i=0;i<NXK_TAREAS_MAX;++i)
        if (tareas[i].estado==NXK_TAREA_ESPERANDO_SEMAFORO && tareas[i].espera_semaforo==id)
            tareas[i].estado=NXK_TAREA_LISTA;
    return 0;
}
int nxk_semaforo_adquirir(uint32_t id,uint32_t ticks) {
    if (actual>=NXK_TAREAS_MAX || ticks>0x7fffffffu || !interrupciones()) return -1;
    struct tarea *t=&tareas[actual]; uint32_t inicio=nxk_reloj_ticks();
    for (;;) {
        struct semaforo *s=buscar_semaforo(id); if (!s) return -1;
        if (s->valor) { --s->valor; return 0; }
        if (!ticks) return 1;
        if ((uint32_t)(nxk_reloj_ticks()-inicio)>=ticks) { incrementar(&t->metricas.plazos); return 1; }
        incrementar(&t->metricas.esperas_semaforo); t->espera_semaforo=id;
        t->despertar=inicio+ticks; t->estado=NXK_TAREA_ESPERANDO_SEMAFORO;
        nxk_cambiar_tarea(&t->pila,principal); t->espera_semaforo=0;
    }
}
int nxk_semaforo_destruir(uint32_t id) {
    struct semaforo *s=buscar_semaforo(id); if (!s) return -1;
    if (referencias_semaforo(id)) return -3;
    s->activo=0; s->valor=s->maximo=0; return 0;
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
            if ((t->estado==NXK_TAREA_DORMIDA || t->estado==NXK_TAREA_ESPERANDO || t->estado==NXK_TAREA_ESPERANDO_FIN ||
                t->estado==NXK_TAREA_ESPERANDO_ENVIO || t->estado==NXK_TAREA_ESPERANDO_SEMAFORO) && vencido(ahora,t->despertar)) t->estado=NXK_TAREA_LISTA;
            if (t->estado==NXK_TAREA_LISTA || t->estado==NXK_TAREA_DORMIDA ||
                t->estado==NXK_TAREA_ESPERANDO || t->estado==NXK_TAREA_ESPERANDO_FIN ||
                t->estado==NXK_TAREA_ESPERANDO_ENVIO || t->estado==NXK_TAREA_ESPERANDO_SEMAFORO) ++pendientes;
            if (elegida==NXK_TAREAS_MAX && t->estado==NXK_TAREA_LISTA) elegida=i;
        }
        if (!pendientes) break;
        if ((uint32_t)(ahora-inicio)>=presupuesto) { resultado=1; break; }
        if (elegida==NXK_TAREAS_MAX) { __asm__ volatile("hlt"); continue; }
        struct tarea *t=&tareas[elegida];
        if (!pila_valida(t)) { terminar(t,NXK_TAREA_CANCELADA,UINT32_MAX); resultado=-5; break; }
        cursor=(elegida+1)%NXK_TAREAS_MAX; actual=elegida;
        t->estado=NXK_TAREA_EJECUTANDO; incrementar(&t->activaciones);
        nxk_cambiar_tarea(&principal,t->pila);
        actual=NXK_TAREAS_MAX;
        if (!pila_valida(t)) { terminar(t,NXK_TAREA_CANCELADA,UINT32_MAX); resultado=-5; break; }
        if (!interrupciones()) { resultado=-4; break; }
    }
    actual=NXK_TAREAS_MAX; organizando=0; return resultado;
}
#ifdef NXK_TEST
int nxk_planificador_probar_resumen(void) {
    struct nxk_planificador_resumen r; memset(&r,0xa5,sizeof(r));
    uint32_t estado=tareas[0].estado; tareas[0].estado=NXK_TAREA_ESTADOS;
    int n=nxk_planificador_consultar(&r); tareas[0].estado=estado;
    if (n!=-2 || nxk_planificador_consultar(0)!=-1) return 1;
    for (unsigned i=0;i<sizeof(r);++i) if (((unsigned char *)&r)[i]!=0xa5) return 2;
    return 0;
}
int nxk_planificador_probar_saturacion(void) {
    uint32_t n=UINT32_MAX-1; incrementar(&n); incrementar(&n);
    return n!=UINT32_MAX;
}
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
int nxk_semaforo_crear(uint32_t i,uint32_t m,uint32_t *id) { (void)i;(void)m;(void)id;return -64; }
int nxk_semaforo_consultar(uint32_t id,struct nxk_semaforo_info *s) { (void)id;(void)s;return -64; }
int nxk_semaforo_publicar(uint32_t id) { (void)id;return -64; }
int nxk_semaforo_adquirir(uint32_t id,uint32_t n) { (void)id;(void)n;return -64; }
int nxk_semaforo_destruir(uint32_t id) { (void)id;return -64; }
int nxk_tarea_crear(void (*f)(void *),void *a,uint32_t *id) { (void)f;(void)a;(void)id;return -64; }
int nxk_tarea_consultar(uint32_t id,struct nxk_tarea_info *i) { (void)id;(void)i;return -64; }
int nxk_tarea_metricas(uint32_t id,struct nxk_tarea_metricas *s) { (void)id;(void)s;return -64; }
int nxk_planificador_consultar(struct nxk_planificador_resumen *s) { (void)s;return -64; }
int nxk_tarea_enumerar(unsigned s,struct nxk_tarea_info *i) { (void)s;(void)i;return -64; }
uint32_t nxk_tarea_actual(void) { return 0; }
int nxk_tarea_ceder(void) { return -64; }
int nxk_tarea_dormir(uint32_t n) { (void)n;return -64; }
int nxk_tarea_enviar(uint32_t id,uint32_t v) { (void)id;(void)v;return -64; }
int nxk_tarea_enviar_esperando(uint32_t id,uint32_t v,uint32_t n) { (void)id;(void)v;(void)n;return -64; }
int nxk_tarea_recibir(struct nxk_mensaje *m) { (void)m;return -64; }
int nxk_tarea_esperar_mensaje(struct nxk_mensaje *m,uint32_t n) { (void)m;(void)n;return -64; }
int nxk_tarea_finalizar(uint32_t c) { (void)c;return -64; }
int nxk_tarea_esperar_fin(uint32_t id,struct nxk_tarea_resultado *s,uint32_t n) { (void)id;(void)s;(void)n;return -64; }
int nxk_tarea_cancelar(uint32_t id) { (void)id;return -64; }
int nxk_tarea_liberar(uint32_t id) { (void)id;return -64; }
int nxk_tareas_ejecutar(uint32_t n) { (void)n;return -64; }
#endif
