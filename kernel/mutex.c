/* Demostraciones y pruebas de propiedad, abandono y ciclos; puente C kernel. */
#include "tareas.h"
#include "mutex.h"
#include "paginacion.h"
#if defined(__i386__)
struct compartido_mutex { uint32_t mutex, ids[NXK_TAREAS_MAX], suma, dentro, fallos; };
static void compartir_mutex(void *argumento) {
    struct compartido_mutex *p=argumento;
    for (unsigned i=0;i<4;++i) {
        if (nxk_mutex_adquirir(p->mutex,50)) { ++p->fallos; return; }
        if (p->dentro++) ++p->fallos;
        uint32_t anterior=p->suma; nxk_tarea_ceder();
        if (p->dentro!=1 || p->suma!=anterior) ++p->fallos;
        p->suma=anterior+1; --p->dentro;
        if (nxk_mutex_liberar(p->mutex)) ++p->fallos;
    }
}
int nxk_mutex_demo(void) {
    struct compartido_mutex p; memset(&p,0,sizeof(p));
    if (nxk_mutex_crear(&p.mutex)) return -1;
    int r=0;
    for (unsigned i=0;i<NXK_TAREAS_MAX;++i) if (nxk_tarea_crear(compartir_mutex,&p,&p.ids[i])) { r=-1; break; }
    if (!r) r=nxk_tareas_ejecutar(100);
    struct nxk_mutex_info info;
    if (nxk_mutex_consultar(p.mutex,&info) || info.propietario || info.abandonado || info.irrecuperable || info.esperadores) r=-1;
    for (unsigned i=0;i<NXK_TAREAS_MAX;++i) nxk_tarea_cancelar(p.ids[i]);
    for (unsigned i=0;i<NXK_TAREAS_MAX;++i) if (p.ids[i] && nxk_tarea_liberar(p.ids[i])) r=-1;
    if (nxk_mutex_destruir(p.mutex)) r=-1;
    return r || p.suma!=32 || p.dentro || p.fallos ? -2 : 0;
}
/* Una tarea deja datos a medio actualizar; otra debe repararlos explicitamente. */
struct recuperacion { uint32_t mutex, ids[2], escrito, esperando, valor, reparado, fallos; };
static void dejar_recurso(void *argumento) {
    struct recuperacion *p=argumento;
    if (nxk_mutex_adquirir(p->mutex,0)) { ++p->fallos; return; }
    p->valor=999; p->escrito=1;
    while (!p->esperando) nxk_tarea_ceder();
    /* Retornar con el mutex tomado lo abandona, sin fingir que los datos estan bien. */
}
static void reparar_recurso(void *argumento) {
    struct recuperacion *p=argumento;
    while (!p->escrito) nxk_tarea_ceder();
    p->esperando=1;
    if (nxk_mutex_adquirir(p->mutex,50)!=2 || p->valor!=999) { ++p->fallos; return; }
    p->valor=0;
    if (nxk_mutex_consistente(p->mutex) || nxk_mutex_liberar(p->mutex) ||
        nxk_mutex_adquirir(p->mutex,0) || nxk_mutex_liberar(p->mutex)) { ++p->fallos; return; }
    p->reparado=1;
}
int nxk_mutex_recuperacion_demo(void) {
    struct recuperacion p; memset(&p,0,sizeof(p));
    if (nxk_mutex_crear(&p.mutex)) return -1;
    int r=nxk_tarea_crear(dejar_recurso,&p,&p.ids[0]);
    if (!r) r=nxk_tarea_crear(reparar_recurso,&p,&p.ids[1]);
    if (!r) r=nxk_tareas_ejecutar(100);
    for (unsigned i=0;i<2;++i) nxk_tarea_cancelar(p.ids[i]);
    for (unsigned i=0;i<2;++i) if (p.ids[i] && nxk_tarea_liberar(p.ids[i])) r=-1;
    if (nxk_mutex_destruir(p.mutex)) r=-1;
    return r || p.fallos || !p.reparado || p.valor ? -2 : 0;
}
#ifdef NXK_TEST
struct prueba_mutex { uint32_t mutex, segundo, propietario, observador, etapa_prop, etapa_obs, retorno, modo, valor, fallos; };
static void propietario_mutex(void *argumento) {
    struct prueba_mutex *p=argumento;
    if (nxk_mutex_adquirir(p->mutex,0) || nxk_mutex_adquirir(p->mutex,0)!=-2 ||
        nxk_mutex_adquirir(0,0)!=-1 || nxk_mutex_consistente(p->mutex)!=-3 ||
        nxk_mutex_destruir(p->mutex)!=-3 || (p->segundo && nxk_mutex_adquirir(p->segundo,0))) ++p->fallos;
    p->valor=999; p->etapa_prop=1;
    if (p->modo==3) {
        while (!p->etapa_obs) nxk_tarea_ceder();
        return; /* Abandono por retorno normal, sin desbloquear. */
    }
    nxk_tarea_dormir(20); p->etapa_prop=2;
    if (nxk_mutex_liberar(p->mutex) || (p->segundo && nxk_mutex_liberar(p->segundo))) ++p->fallos;
}
static void observar_mutex(void *argumento) {
    struct prueba_mutex *p=argumento;
    while (!p->etapa_prop) nxk_tarea_ceder();
    if (nxk_mutex_liberar(p->mutex)!=-2 || nxk_mutex_consistente(p->mutex)!=-2 ||
        nxk_mutex_adquirir(p->mutex,0)!=1 || nxk_mutex_adquirir(p->mutex,0x80000000u)!=-1) ++p->fallos;
    p->etapa_obs=1;
    p->retorno=(uint32_t)nxk_mutex_adquirir(p->mutex,p->modo ? 50 : 2);
    p->etapa_obs=2;
    if (!p->modo) { if (p->retorno!=1) ++p->fallos; return; }
    if (p->retorno!=2) { ++p->fallos; return; }
    struct nxk_mutex_info info;
    if (nxk_mutex_consultar(p->mutex,&info) || info.propietario!=nxk_tarea_actual() || !info.abandonado) ++p->fallos;
    if (p->modo!=2) {
        p->valor=0; /* Reparacion explicita de los datos de esta prueba. */
        if (nxk_mutex_consistente(p->mutex) || nxk_mutex_consistente(p->mutex)!=-3) ++p->fallos;
    }
    if (nxk_mutex_liberar(p->mutex)) ++p->fallos;
    if (p->modo==2) { if (nxk_mutex_adquirir(p->mutex,0)!=-5) ++p->fallos; }
    else if (nxk_mutex_adquirir(p->mutex,0) || nxk_mutex_liberar(p->mutex)) ++p->fallos;
    if (p->segundo && (nxk_mutex_adquirir(p->segundo,0)!=2 || nxk_mutex_consistente(p->segundo) || nxk_mutex_liberar(p->segundo))) ++p->fallos;
}
static int caso_abandono(uint32_t modo) {
    struct prueba_mutex p; memset(&p,0,sizeof(p)); p.modo=modo;
    if (nxk_mutex_crear(&p.mutex) || (modo==1 && nxk_mutex_crear(&p.segundo)) ||
        nxk_tarea_crear(propietario_mutex,&p,&p.propietario) || nxk_tarea_crear(observar_mutex,&p,&p.observador)) return 1;
    if (modo==1 || modo==2) {
        struct nxk_tarea_info t; struct nxk_mutex_info m;
        if (nxk_tareas_ejecutar(1)!=1 || p.etapa_obs!=1 || nxk_tarea_consultar(p.observador,&t) ||
            t.estado!=NXK_TAREA_ESPERANDO_MUTEX || t.esperando!=p.mutex || nxk_tarea_cancelar(p.propietario) ||
            nxk_mutex_consultar(p.mutex,&m) || m.propietario || !m.abandonado || m.esperadores!=1 ||
            nxk_mutex_destruir(p.mutex)!=-3) return 2;
    }
    if (nxk_tareas_ejecutar(100) || p.fallos || p.etapa_obs!=2) return 3;
    struct nxk_mutex_info m;
    if (nxk_mutex_consultar(p.mutex,&m) || m.propietario || m.esperadores ||
        (modo==2 ? !m.irrecuperable : m.irrecuperable || m.abandonado) ||
        (modo==1 && p.valor)) return 4;
    if (nxk_tarea_liberar(p.propietario) || nxk_tarea_liberar(p.observador) ||
        nxk_mutex_destruir(p.mutex) || (p.segundo && nxk_mutex_destruir(p.segundo))) return 5;
    return 0;
}
struct ciclo_mutex { uint32_t mutex[2], ids[2], tomados, mixto, fallos; int r[2]; };
static void entrar_ciclo(void *argumento) {
    struct ciclo_mutex *p=argumento; unsigned i=nxk_tarea_actual()==p->ids[0] ? 0 : 1;
    if ((!p->mixto || !i) && nxk_mutex_adquirir(p->mutex[i],0)) { ++p->fallos; return; }
    p->tomados|=1u<<i; while (p->tomados!=3) nxk_tarea_ceder();
    if (p->mixto && !i) {
        struct nxk_tarea_resultado r; p->r[i]=nxk_tarea_esperar_fin(p->ids[1],&r,50);
    } else {
        unsigned otro=p->mixto ? 0 : 1-i;
        p->r[i]=nxk_mutex_adquirir(p->mutex[otro],50);
        if (!p->r[i] && nxk_mutex_liberar(p->mutex[otro])) ++p->fallos;
    }
    if ((!p->mixto || !i) && nxk_mutex_liberar(p->mutex[i])) ++p->fallos;
}
static int probar_ciclo_mutex(uint32_t mixto) {
    struct ciclo_mutex p; memset(&p,0,sizeof(p)); p.mixto=mixto;
    for (unsigned i=0;i<2;++i) if (nxk_mutex_crear(&p.mutex[i]) || nxk_tarea_crear(entrar_ciclo,&p,&p.ids[i])) return 1;
    if (nxk_tareas_ejecutar(100) || p.fallos) return 2;
    if (mixto) { if (!((p.r[0]==0 && p.r[1]==-4) || (p.r[0]==-2 && p.r[1]==0))) return 3; }
    else if (!((p.r[0]==0 && p.r[1]==-4) || (p.r[0]==-4 && p.r[1]==0))) return 3;
    for (unsigned i=0;i<2;++i) if (nxk_tarea_liberar(p.ids[i]) || nxk_mutex_destruir(p.mutex[i])) return 4;
    return 0;
}
int nxk_mutex_probar(void) {
    uint32_t ids[NXK_MUTEX_MAX], salida=0xdeadbeefu; struct nxk_mutex_info info;
    if (nxk_mutex_crear(0)!=-1 || nxk_mutex_adquirir(0,1)!=-1 || nxk_mutex_liberar(0)!=-1) return 1;
    for (unsigned i=0;i<NXK_MUTEX_MAX;++i) if (nxk_mutex_crear(&ids[i]) || nxk_mutex_consultar(ids[i],&info) ||
        info.propietario || info.abandonado || info.irrecuperable || info.esperadores) return 2;
    if (nxk_mutex_crear(&salida)!=-2 || salida!=0xdeadbeefu) return 3;
    for (unsigned i=0;i<NXK_MUTEX_MAX;++i) if (nxk_mutex_destruir(ids[i])) return 4;
    if (nxk_mutex_crear(&salida) || salida==ids[0] || nxk_mutex_destruir(ids[0])!=-1 ||
        nxk_mutex_consultar(salida,0)!=-1 || nxk_mutex_destruir(salida)) return 5;
    memset(&info,0xa5,sizeof(info));
    if (nxk_mutex_consultar(salida,&info)!=-1) return 6;
    for (unsigned i=0;i<sizeof(info);++i) if (((unsigned char *)&info)[i]!=0xa5) return 6;
    for (unsigned modo=0;modo<4;++modo) if (caso_abandono(modo)) return 10+(int)modo;
    /* Cancelar un esperador elimina su referencia sin ejecutar continuacion. */
    struct prueba_mutex p; memset(&p,0,sizeof(p)); p.modo=1;
    if (nxk_mutex_crear(&p.mutex) || nxk_tarea_crear(propietario_mutex,&p,&p.propietario) ||
        nxk_tarea_crear(observar_mutex,&p,&p.observador) || nxk_tareas_ejecutar(1)!=1 || p.etapa_obs!=1 ||
        nxk_tarea_cancelar(p.observador) || nxk_tarea_cancelar(p.propietario) || nxk_mutex_destruir(p.mutex) ||
        nxk_tareas_ejecutar(100) || p.etapa_obs!=1 || nxk_tarea_liberar(p.propietario) || nxk_tarea_liberar(p.observador)) return 14;
    if (probar_ciclo_mutex(0) || probar_ciclo_mutex(1)) return 15;
    size_t usado=nxk_heap_used(); uint32_t guardas=nxk_paginacion_guardas();
    if (nxk_mutex_demo() || nxk_mutex_demo() || nxk_mutex_recuperacion_demo() ||
        nxk_mutex_recuperacion_demo() || nxk_heap_used()!=usado || nxk_paginacion_guardas()!=guardas) return 16;
    return 0;
}
#endif
#else
int nxk_mutex_recuperacion_demo(void) { return -64; }
int nxk_mutex_demo(void) { return -64; }
#endif
