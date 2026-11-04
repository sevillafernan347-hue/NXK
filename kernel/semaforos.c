/* Demostracion y pruebas de semaforos kernel; puente C sobre planificador NXA. */
#include "tareas.h"
#include "semaforos.h"
#include "paginacion.h"
#if defined(__i386__)
struct compartido { uint32_t semaforo, ids[NXK_TAREAS_MAX], suma, dentro, fallos; };
static void compartir(void *argumento) {
    struct compartido *p=argumento;
    for (unsigned i=0;i<4;++i) {
        if (nxk_semaforo_adquirir(p->semaforo,50)) { ++p->fallos; return; }
        if (p->dentro++) ++p->fallos;
        uint32_t anterior=p->suma;
        nxk_tarea_ceder(); /* Los demas deben esperar aun cuando este cede. */
        if (p->dentro!=1 || p->suma!=anterior) ++p->fallos;
        p->suma=anterior+1; --p->dentro;
        if (nxk_semaforo_publicar(p->semaforo)) ++p->fallos;
    }
}
int nxk_semaforos_demo(void) {
    struct compartido p; memset(&p,0,sizeof(p));
    if (nxk_semaforo_crear(1,1,&p.semaforo)) return -1;
    int r=0;
    for (unsigned i=0;i<NXK_TAREAS_MAX;++i)
        if (nxk_tarea_crear(compartir,&p,&p.ids[i])) { r=-1; break; }
    if (!r) r=nxk_tareas_ejecutar(100);
    for (unsigned i=0;i<NXK_TAREAS_MAX;++i) nxk_tarea_cancelar(p.ids[i]);
    for (unsigned i=0;i<NXK_TAREAS_MAX;++i) if (p.ids[i] && nxk_tarea_liberar(p.ids[i])) r=-1;
    if (nxk_semaforo_destruir(p.semaforo)) r=-1;
    return r || p.suma!=32 || p.dentro || p.fallos ? -2 : 0;
}
#ifdef NXK_TEST
struct espera { uint32_t semaforo, fase, retorno, plazo, fallos; };
static void esperar_permiso(void *argumento) {
    struct espera *p=argumento; p->fase=1;
    p->retorno=(uint32_t)nxk_semaforo_adquirir(p->semaforo,p->plazo); p->fase=2;
}
static void contar_permisos(void *argumento) {
    struct espera *p=argumento; struct nxk_semaforo_info info;
    if (nxk_semaforo_adquirir(p->semaforo,0) || nxk_semaforo_adquirir(p->semaforo,0) ||
        nxk_semaforo_adquirir(p->semaforo,0)!=1 || nxk_semaforo_consultar(p->semaforo,&info) ||
        info.disponibles || info.esperadores || nxk_semaforo_publicar(p->semaforo) ||
        nxk_semaforo_publicar(p->semaforo) || nxk_semaforo_publicar(p->semaforo)!=-2 ||
        nxk_semaforo_adquirir(p->semaforo,0x80000000u)!=-1 || nxk_semaforo_adquirir(0,0)!=-1) ++p->fallos;
}
int nxk_semaforos_probar(void) {
    uint32_t ids[NXK_SEMAFOROS_MAX], salida=0xdeadbeefu, tarea;
    struct nxk_semaforo_info info;
    if (nxk_semaforo_crear(1,0,&salida)!=-1 || nxk_semaforo_crear(2,1,&salida)!=-1 ||
        nxk_semaforo_crear(0,1,0)!=-1 || salida!=0xdeadbeefu || nxk_semaforo_adquirir(0,1)!=-1) return 1;
    for (unsigned i=0;i<NXK_SEMAFOROS_MAX;++i)
        if (nxk_semaforo_crear(1,1,&ids[i]) || nxk_semaforo_consultar(ids[i],&info) ||
            info.disponibles!=1 || info.maximo!=1 || info.esperadores) return 2;
    if (nxk_semaforo_crear(1,1,&salida)!=-2 || salida!=0xdeadbeefu) return 3;
    for (unsigned i=0;i<NXK_SEMAFOROS_MAX;++i) if (nxk_semaforo_destruir(ids[i])) return 4;
    if (nxk_semaforo_crear(UINT32_MAX,UINT32_MAX,&salida) || salida==ids[0] ||
        nxk_semaforo_publicar(salida)!=-2 || nxk_semaforo_publicar(ids[0])!=-1 ||
        nxk_semaforo_destruir(ids[0])!=-1 || nxk_semaforo_consultar(salida,0)!=-1 ||
        nxk_semaforo_destruir(salida)) return 5;
    memset(&info,0xa5,sizeof(info));
    if (nxk_semaforo_consultar(salida,&info)!=-1) return 6;
    for (unsigned i=0;i<sizeof(info);++i) if (((unsigned char *)&info)[i]!=0xa5) return 6;
    struct espera p; memset(&p,0,sizeof(p));
    if (nxk_semaforo_crear(2,2,&p.semaforo) || nxk_tarea_crear(contar_permisos,&p,&tarea) ||
        nxk_tareas_ejecutar(100) || p.fallos || nxk_tarea_liberar(tarea) || nxk_semaforo_destruir(p.semaforo)) return 7;
    p.fase=0; p.plazo=2;
    if (nxk_semaforo_crear(0,1,&p.semaforo) || nxk_tarea_crear(esperar_permiso,&p,&tarea) ||
        nxk_tareas_ejecutar(100) || p.fase!=2 || p.retorno!=1 || nxk_semaforo_consultar(p.semaforo,&info) ||
        info.disponibles || info.esperadores) return 8;
    struct nxk_tarea_metricas m;
    if (nxk_tarea_metricas(tarea,&m) || m.esperas_semaforo!=1 || m.plazos!=1 ||
        nxk_tarea_liberar(tarea) || nxk_semaforo_destruir(p.semaforo)) return 9;
    p.fase=0; p.plazo=20;
    if (nxk_semaforo_crear(0,1,&p.semaforo) || nxk_tarea_crear(esperar_permiso,&p,&tarea) ||
        nxk_tareas_ejecutar(1)!=1 || p.fase!=1 || nxk_semaforo_consultar(p.semaforo,&info) ||
        info.esperadores!=1 || nxk_semaforo_destruir(p.semaforo)!=-3 ||
        nxk_semaforo_publicar(p.semaforo) || nxk_semaforo_destruir(p.semaforo)!=-3 ||
        nxk_tareas_ejecutar(100) || p.fase!=2 || p.retorno || nxk_semaforo_consultar(p.semaforo,&info) ||
        info.disponibles || info.esperadores || nxk_tarea_liberar(tarea) || nxk_semaforo_destruir(p.semaforo)) return 10;
    p.fase=0;
    if (nxk_semaforo_crear(0,1,&p.semaforo) || nxk_tarea_crear(esperar_permiso,&p,&tarea) ||
        nxk_tareas_ejecutar(1)!=1 || p.fase!=1 || nxk_tarea_cancelar(tarea) ||
        nxk_semaforo_destruir(p.semaforo) || nxk_tareas_ejecutar(100) || p.fase!=1 || nxk_tarea_liberar(tarea)) return 11;
    size_t usado=nxk_heap_used(); uint32_t guardas=nxk_paginacion_guardas();
    if (nxk_semaforos_demo() || nxk_semaforos_demo() || nxk_heap_used()!=usado ||
        nxk_paginacion_guardas()!=guardas) return 12;
    return 0;
}
#endif
#else
int nxk_semaforos_demo(void) { return -64; }
#endif
