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
/* Receptor bloqueado y emisor con contrapresion: sin consultar buzon en vacio. */
struct dialogo { uint32_t receptor, emisor, recibidos, fallos; };
static void recibir_dialogo(void *argumento) {
    struct dialogo *d=argumento;
    for (unsigned i=0;i<16;++i) {
        struct nxk_mensaje m;
        if (nxk_tarea_esperar_mensaje(&m,50) || m.origen!=d->emisor || m.valor!=70+i) { ++d->fallos; return; }
        ++d->recibidos;
    }
}
static void enviar_dialogo(void *argumento) {
    struct dialogo *d=argumento; struct nxk_tarea_info info;
    /* Asegurar que el primer envio realmente despierte a una tarea esperando. */
    for (;;) {
        if (nxk_tarea_consultar(d->receptor,&info)) { ++d->fallos; return; }
        if (info.estado==NXK_TAREA_ESPERANDO) break;
        if (info.estado==NXK_TAREA_TERMINADA) { ++d->fallos; return; }
        nxk_tarea_ceder();
    }
    for (unsigned i=0;i<16;++i) {
        int r=nxk_tarea_enviar_esperando(d->receptor,70+i,50);
        if (r) { ++d->fallos; return; }
        if (!i && (nxk_tarea_consultar(d->receptor,&info) || info.estado!=NXK_TAREA_LISTA)) ++d->fallos;
    }
}
int nxk_tareas_mensajes_demo(void) {
    struct dialogo d; memset(&d,0,sizeof(d));
    if (nxk_tarea_crear(recibir_dialogo,&d,&d.receptor)) return -1;
    if (nxk_tarea_crear(enviar_dialogo,&d,&d.emisor)) {
        nxk_tarea_cancelar(d.receptor); nxk_tarea_liberar(d.receptor); return -1;
    }
    int r=nxk_tareas_ejecutar(100);
    nxk_tarea_cancelar(d.receptor); nxk_tarea_cancelar(d.emisor);
    nxk_tarea_liberar(d.receptor); nxk_tarea_liberar(d.emisor);
    return r || d.fallos || d.recibidos!=16 ? -2 : 0;
}
/* Dos observadores esperan el mismo resultado sin consumirlo. */
struct resultados_demo { uint32_t trabajador, observadores[2], vistos, fallos; };
static void calcular_resultado(void *argumento) {
    struct resultados_demo *d=argumento;
    for (;;) {
        struct nxk_tarea_info a,b;
        if (nxk_tarea_consultar(d->observadores[0],&a) || nxk_tarea_consultar(d->observadores[1],&b)) { ++d->fallos; return; }
        if (a.estado==NXK_TAREA_ESPERANDO_FIN && b.estado==NXK_TAREA_ESPERANDO_FIN) break;
        nxk_tarea_ceder();
    }
    nxk_tarea_finalizar(42);
    ++d->fallos; /* Finalizar correctamente nunca debe volver aqui. */
}
static void observar_resultado(void *argumento) {
    struct resultados_demo *d=argumento; struct nxk_tarea_resultado r;
    if (nxk_tarea_esperar_fin(d->trabajador,&r,50) || r.estado!=NXK_TAREA_TERMINADA || r.codigo!=42) { ++d->fallos; return; }
    ++d->vistos;
    if (d->vistos==1 && nxk_tarea_liberar(d->trabajador)!=-3) ++d->fallos;
    if (nxk_tarea_esperar_fin(d->trabajador,&r,0) || r.codigo!=42) ++d->fallos;
}
int nxk_tareas_resultados_demo(void) {
    struct resultados_demo d; memset(&d,0,sizeof(d));
    int fallo=nxk_tarea_crear(calcular_resultado,&d,&d.trabajador);
    for (unsigned i=0;!fallo && i<2;++i) fallo=nxk_tarea_crear(observar_resultado,&d,&d.observadores[i]);
    if (!fallo) fallo=nxk_tareas_ejecutar(100);
    /* Cancelar todas las dependencias antes de liberar cualquier destino. */
    nxk_tarea_cancelar(d.trabajador);
    for (unsigned i=0;i<2;++i) nxk_tarea_cancelar(d.observadores[i]);
    nxk_tarea_liberar(d.trabajador);
    for (unsigned i=0;i<2;++i) nxk_tarea_liberar(d.observadores[i]);
    return fallo || d.fallos || d.vistos!=2 ? -1 : 0;
}
/* Buzones llenos y dependencias en anillo: rechazar sin esperar al timeout. */
struct bloqueos_demo { uint32_t ids[NXK_TAREAS_MAX], cantidad, mixto, fallos; int retornos[NXK_TAREAS_MAX]; };
static void esperar_en_anillo(void *argumento) {
    struct bloqueos_demo *d=argumento; unsigned i=0;
    while (i<d->cantidad && d->ids[i]!=nxk_tarea_actual()) ++i;
    if (i==d->cantidad) { ++d->fallos; return; }
    uint32_t destino=d->ids[(i+1)%d->cantidad];
    if (d->mixto && !i) {
        struct nxk_tarea_resultado r;
        d->retornos[i]=nxk_tarea_esperar_fin(destino,&r,50);
        if (!d->retornos[i] && (r.estado!=NXK_TAREA_TERMINADA || r.codigo)) ++d->fallos;
    } else d->retornos[i]=nxk_tarea_enviar_esperando(destino,999,50);
    /* Ningun ciclo rechazado puede haber cambiado el contenido del buzon. */
    for (unsigned j=0;j<NXK_BUZON_MAX;++j) {
        struct nxk_mensaje m;
        if (nxk_tarea_recibir(&m) || m.origen || m.valor!=j) ++d->fallos;
    }
}
static int caso_bloqueos(unsigned cantidad,unsigned mixto) {
    struct bloqueos_demo d; memset(&d,0,sizeof(d)); d.cantidad=cantidad; d.mixto=mixto;
    int fallo=0;
    for (unsigned i=0;i<cantidad && !fallo;++i) {
        d.retornos[i]=99;
        if (nxk_tarea_crear(esperar_en_anillo,&d,&d.ids[i])) { fallo=1; break; }
        for (unsigned j=0;j<NXK_BUZON_MAX;++j) if (nxk_tarea_enviar(d.ids[i],j)) fallo=1;
    }
    if (!fallo) fallo=nxk_tareas_ejecutar(100);
    if (!fallo && mixto) {
        if (!((d.retornos[0]==0 && d.retornos[1]==-4) ||
              (d.retornos[0]==-2 && d.retornos[1]==-3))) fallo=1;
    } else if (!fallo) {
        unsigned rechazados=0;
        for (unsigned i=0;i<cantidad;++i) {
            if (d.retornos[i]==-4) ++rechazados;
            else if (d.retornos[i]!=-3) fallo=1;
        }
        if (rechazados!=1) fallo=1;
    }
    for (unsigned i=0;i<cantidad;++i) nxk_tarea_cancelar(d.ids[i]);
    for (unsigned i=0;i<cantidad;++i) if (d.ids[i] && nxk_tarea_liberar(d.ids[i])) fallo=1;
    return fallo || d.fallos;
}
int nxk_tareas_bloqueos_demo(void) {
    return caso_bloqueos(2,0) || caso_bloqueos(8,0) || caso_bloqueos(2,1);
}
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
    for (unsigned i=0;i<NXK_BUZON_MAX;++i) if (nxk_tarea_enviar(nxk_tarea_actual(),i)) ++errores;
    if (nxk_tarea_enviar_esperando(nxk_tarea_actual(),99,0)!=1 ||
        nxk_tarea_enviar_esperando(nxk_tarea_actual(),99,10)!=-4) ++errores;
    for (unsigned i=0;i<NXK_BUZON_MAX;++i)
        if (nxk_tarea_recibir(&m) || m.origen!=nxk_tarea_actual() || m.valor!=i) ++errores;
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
struct espera_prueba { uint32_t fase, fallos; };
static void probar_plazo(void *argumento) {
    struct espera_prueba *p=argumento; struct nxk_mensaje m={99,88};
    uint32_t antes=nxk_reloj_ticks();
    if (nxk_tarea_esperar_mensaje(&m,3)!=1 || nxk_reloj_ticks()-antes<3 ||
        m.origen!=99 || m.valor!=88 || nxk_tarea_esperar_mensaje(&m,0)!=1 ||
        nxk_tarea_esperar_mensaje(&m,0x80000000u)!=-1 || nxk_tarea_esperar_mensaje(0,1)!=-1) ++p->fallos;
    /* Una consulta de cero ticks si recoge un mensaje ya disponible. */
    if (nxk_tarea_enviar(nxk_tarea_actual(),77) || nxk_tarea_esperar_mensaje(&m,0) ||
        m.valor!=77 || m.origen!=nxk_tarea_actual()) ++p->fallos;
    p->fase=2;
}
static void probar_reanudacion(void *argumento) {
    struct espera_prueba *p=argumento; struct nxk_mensaje m;
    p->fase=1;
    if (nxk_tarea_esperar_mensaje(&m,20) || m.valor!=4321 || m.origen) ++p->fallos;
    p->fase=2;
}
static int probar_esperas(void) {
    struct espera_prueba p={0,0}; struct nxk_mensaje m; struct nxk_tarea_info info;
    uint32_t id;
    if (nxk_tarea_esperar_mensaje(&m,1)!=-1 ||
        nxk_tarea_crear(probar_plazo,&p,&id) || nxk_tareas_ejecutar(100) ||
        p.fallos || p.fase!=2 || nxk_tarea_liberar(id)) return 1;
    p.fase=p.fallos=0;
    if (nxk_tarea_crear(probar_reanudacion,&p,&id) || nxk_tareas_ejecutar(1)!=1 ||
        p.fase!=1 || nxk_tarea_consultar(id,&info) || info.estado!=NXK_TAREA_ESPERANDO ||
        nxk_tarea_enviar(id,4321) || nxk_tarea_consultar(id,&info) ||
        info.estado!=NXK_TAREA_LISTA || nxk_tareas_ejecutar(100) || p.fallos ||
        p.fase!=2 || nxk_tarea_liberar(id)) return 2;
    p.fase=p.fallos=0;
    if (nxk_tarea_crear(probar_reanudacion,&p,&id) || nxk_tareas_ejecutar(1)!=1 ||
        p.fase!=1 || nxk_tarea_cancelar(id) || nxk_tarea_enviar(id,4321)!=-3 ||
        nxk_tareas_ejecutar(100) || p.fase!=1 || nxk_tarea_liberar(id)) return 3;
    return nxk_tareas_mensajes_demo() || nxk_tareas_mensajes_demo() ? 4 : 0;
}
struct union_prueba { uint32_t destino, fase, fallos, retorno; struct nxk_tarea_resultado resultado; };
static void esperar_resultado(void *argumento) {
    struct union_prueba *p=argumento; p->fase=1;
    p->retorno=(uint32_t)nxk_tarea_esperar_fin(p->destino,&p->resultado,50); p->fase=2;
}
static void plazo_resultado(void *argumento) {
    struct union_prueba *p=argumento; struct nxk_tarea_resultado r={88,99};
    if (nxk_tarea_esperar_fin(p->destino,&r,0)!=1 || r.estado!=88 || r.codigo!=99 ||
        nxk_tarea_esperar_fin(p->destino,0,1)!=-1 || nxk_tarea_esperar_fin(p->destino,&r,0x80000000u)!=-1 ||
        nxk_tarea_esperar_fin(0,&r,1)!=-1 || nxk_tarea_esperar_fin(nxk_tarea_actual(),&r,1)!=-2) ++p->fallos;
    uint32_t antes=nxk_reloj_ticks();
    if (nxk_tarea_esperar_fin(p->destino,&r,2)!=1 || nxk_reloj_ticks()-antes<2 ||
        r.estado!=88 || r.codigo!=99) ++p->fallos;
    p->fase=2;
}
static void dormir_largo(void *p) { (void)p; nxk_tarea_dormir(20); }
struct ciclo_prueba { uint32_t ids[2], bloqueos, fallos; };
static void probar_ciclo(void *argumento) {
    struct ciclo_prueba *p=argumento; struct nxk_tarea_resultado r;
    uint32_t otro=nxk_tarea_actual()==p->ids[0] ? p->ids[1] : p->ids[0];
    int n=nxk_tarea_esperar_fin(otro,&r,50);
    if (n==-2) ++p->bloqueos;
    else if (n || r.estado!=NXK_TAREA_TERMINADA || r.codigo) ++p->fallos;
}
/* Ocho dependencias en cadena deben resolverse sin perder referencias. */
struct cadena_prueba { uint32_t ids[NXK_TAREAS_MAX], fallos; };
static void eslabon(void *argumento) {
    struct cadena_prueba *p=argumento; unsigned i=0;
    while (i<NXK_TAREAS_MAX && p->ids[i]!=nxk_tarea_actual()) ++i;
    if (i==NXK_TAREAS_MAX) { ++p->fallos; return; }
    if (i==NXK_TAREAS_MAX-1) {
        for (;;) {
            unsigned esperando=0;
            for (unsigned j=0;j<i;++j) {
                struct nxk_tarea_info info;
                if (nxk_tarea_consultar(p->ids[j],&info) || info.estado==NXK_TAREA_TERMINADA) { ++p->fallos; return; }
                if (info.estado==NXK_TAREA_ESPERANDO_FIN) ++esperando;
            }
            if (esperando==i) break;
            nxk_tarea_ceder();
        }
        nxk_tarea_finalizar(100);
    } else {
        struct nxk_tarea_resultado r;
        if (nxk_tarea_esperar_fin(p->ids[i+1],&r,50) || r.estado!=NXK_TAREA_TERMINADA ||
            r.codigo!=106-i) { ++p->fallos; return; }
        nxk_tarea_finalizar(r.codigo+1);
    }
    ++p->fallos;
}
static int probar_cadena(void) {
    struct cadena_prueba p; memset(&p,0,sizeof(p));
    for (unsigned i=0;i<NXK_TAREAS_MAX;++i) if (nxk_tarea_crear(eslabon,&p,&p.ids[i])) return 1;
    if (nxk_tareas_ejecutar(100) || p.fallos) return 2;
    for (unsigned i=0;i<NXK_TAREAS_MAX;++i) {
        struct nxk_tarea_info info;
        if (nxk_tarea_consultar(p.ids[i],&info) || info.estado!=NXK_TAREA_TERMINADA ||
            info.resultado!=107-i || info.esperando || nxk_tarea_liberar(p.ids[i])) return 3;
    }
    return 0;
}
static int probar_resultados(void) {
    struct union_prueba p; memset(&p,0,sizeof(p)); uint32_t observador;
    if (nxk_tarea_finalizar(1)!=-1 || nxk_tarea_esperar_fin(0,&p.resultado,1)!=-1) return 1;
    if (nxk_tareas_resultados_demo() || nxk_tareas_resultados_demo()) return 2;
    if (nxk_tarea_crear(dormir_largo,0,&p.destino) || nxk_tarea_crear(plazo_resultado,&p,&observador) ||
        nxk_tareas_ejecutar(100) || p.fase!=2 || p.fallos || nxk_tarea_liberar(p.destino) ||
        nxk_tarea_liberar(observador)) return 3;
    memset(&p,0,sizeof(p));
    if (nxk_tarea_crear(dormir_largo,0,&p.destino) || nxk_tarea_crear(esperar_resultado,&p,&observador) ||
        nxk_tareas_ejecutar(1)!=1 || p.fase!=1) return 4;
    struct nxk_tarea_info info;
    if (nxk_tarea_consultar(observador,&info) || info.estado!=NXK_TAREA_ESPERANDO_FIN ||
        info.esperando!=p.destino || nxk_tarea_cancelar(p.destino) ||
        nxk_tarea_liberar(p.destino)!=-3 || nxk_tareas_ejecutar(100) || p.fase!=2 || p.retorno ||
        p.resultado.estado!=NXK_TAREA_CANCELADA || p.resultado.codigo!=UINT32_MAX ||
        nxk_tarea_liberar(p.destino) || nxk_tarea_liberar(observador)) return 5;
    memset(&p,0,sizeof(p));
    if (nxk_tarea_crear(dormir_largo,0,&p.destino) || nxk_tarea_crear(esperar_resultado,&p,&observador) ||
        nxk_tareas_ejecutar(1)!=1 || p.fase!=1 || nxk_tarea_cancelar(observador) ||
        nxk_tarea_cancelar(p.destino) || nxk_tarea_liberar(p.destino) || nxk_tarea_liberar(observador) ||
        nxk_tareas_ejecutar(100) || p.fase!=1) return 6;
    struct ciclo_prueba ciclo; memset(&ciclo,0,sizeof(ciclo));
    if (nxk_tarea_crear(probar_ciclo,&ciclo,&ciclo.ids[0]) || nxk_tarea_crear(probar_ciclo,&ciclo,&ciclo.ids[1]) ||
        nxk_tareas_ejecutar(100) || ciclo.bloqueos!=1 || ciclo.fallos ||
        nxk_tarea_liberar(ciclo.ids[0]) || nxk_tarea_liberar(ciclo.ids[1])) return 7;
    return 0;
}
struct envio_prueba { uint32_t destino, emisor, fase, retorno, plazo, fallos; };
static void emitir_esperando(void *argumento) {
    struct envio_prueba *p=argumento; p->fase=1;
    p->retorno=(uint32_t)nxk_tarea_enviar_esperando(p->destino,999,p->plazo); p->fase=2;
}
static void vaciar_despues(void *argumento) {
    struct envio_prueba *p=argumento; nxk_tarea_dormir(3);
    struct nxk_tarea_info info; struct nxk_mensaje m;
    if (nxk_tarea_consultar(p->emisor,&info) || info.estado!=NXK_TAREA_ESPERANDO_ENVIO ||
        info.esperando!=p->destino) ++p->fallos;
    for (unsigned i=0;i<4;++i)
        if (nxk_tarea_recibir(&m) || m.origen || m.valor!=i) ++p->fallos;
    if (nxk_tarea_esperar_mensaje(&m,20) || m.origen!=p->emisor || m.valor!=999) ++p->fallos;
}
static void revisar_sin_envio(void *argumento) {
    struct envio_prueba *p=argumento; nxk_tarea_dormir(10); struct nxk_mensaje m;
    for (unsigned i=0;i<4;++i)
        if (nxk_tarea_recibir(&m) || m.origen || m.valor!=i) ++p->fallos;
    if (nxk_tarea_recibir(&m)!=-2) ++p->fallos;
}
static void contrato_envio(void *argumento) {
    struct envio_prueba *p=argumento;
    if (nxk_tarea_enviar_esperando(0,1,1)!=-1 ||
        nxk_tarea_enviar_esperando(p->destino,999,0x80000000u)!=-1 ||
        nxk_tarea_enviar_esperando(p->destino,999,0)!=1) ++p->fallos;
    uint32_t antes=nxk_reloj_ticks();
    if (nxk_tarea_enviar_esperando(p->destino,999,2)!=1 || nxk_reloj_ticks()-antes<2) ++p->fallos;
}
static int preparar_envio(struct envio_prueba *p,void (*receptor)(void *),void (*emisor)(void *)) {
    memset(p,0,sizeof(*p)); p->plazo=20;
    if (nxk_tarea_crear(receptor,p,&p->destino) || nxk_tarea_crear(emisor,p,&p->emisor)) return 1;
    for (unsigned i=0;i<4;++i) if (nxk_tarea_enviar(p->destino,i)) return 2;
    return 0;
}
struct flujo_multiple { uint32_t emisores[2], receptor, enviados[2], recibidos[2], fallos; };
static void producir_flujo(void *argumento) {
    struct flujo_multiple *p=argumento; unsigned cual=nxk_tarea_actual()==p->emisores[0] ? 0 : 1;
    for (unsigned i=0;i<16;++i) {
        if (nxk_tarea_enviar_esperando(p->receptor,1000*cual+i,50)) { ++p->fallos; return; }
        ++p->enviados[cual];
    }
}
static void consumir_flujo(void *argumento) {
    struct flujo_multiple *p=argumento;
    for (unsigned i=0;i<32;++i) {
        struct nxk_mensaje m;
        if (nxk_tarea_esperar_mensaje(&m,50)) { ++p->fallos; return; }
        unsigned cual=m.origen==p->emisores[0] ? 0 : 1;
        if (m.origen!=p->emisores[cual] || m.valor!=1000*cual+p->recibidos[cual]) { ++p->fallos; return; }
        ++p->recibidos[cual];
    }
}
static int probar_flujo_multiple(void) {
    struct flujo_multiple p; memset(&p,0,sizeof(p));
    if (nxk_tarea_crear(consumir_flujo,&p,&p.receptor)) return 1;
    for (unsigned i=0;i<2;++i) if (nxk_tarea_crear(producir_flujo,&p,&p.emisores[i])) return 1;
    if (nxk_tareas_ejecutar(100) || p.fallos || p.enviados[0]!=16 || p.enviados[1]!=16 ||
        p.recibidos[0]!=16 || p.recibidos[1]!=16) return 2;
    return nxk_tarea_liberar(p.receptor) || nxk_tarea_liberar(p.emisores[0]) || nxk_tarea_liberar(p.emisores[1]);
}
static int probar_envio_esperando(void) {
    struct envio_prueba p; struct nxk_tarea_info info;
    if (nxk_tarea_enviar_esperando(0,1,1)!=-1 || preparar_envio(&p,vaciar_despues,emitir_esperando) ||
        nxk_tareas_ejecutar(100) || p.fallos || p.retorno || p.fase!=2 ||
        nxk_tarea_liberar(p.destino) || nxk_tarea_liberar(p.emisor)) return 1;
    if (preparar_envio(&p,revisar_sin_envio,contrato_envio) || nxk_tareas_ejecutar(100) ||
        p.fallos || nxk_tarea_liberar(p.destino) || nxk_tarea_liberar(p.emisor)) return 2;
    if (preparar_envio(&p,dormir_largo,emitir_esperando) || nxk_tareas_ejecutar(1)!=1 ||
        p.fase!=1 || nxk_tarea_consultar(p.emisor,&info) || info.estado!=NXK_TAREA_ESPERANDO_ENVIO ||
        nxk_tarea_cancelar(p.destino) || nxk_tareas_ejecutar(100) || p.fase!=2 ||
        p.retorno!=(uint32_t)-3 || nxk_tarea_liberar(p.destino) || nxk_tarea_liberar(p.emisor)) return 3;
    if (preparar_envio(&p,dormir_largo,emitir_esperando) || nxk_tareas_ejecutar(1)!=1 || p.fase!=1 ||
        nxk_tarea_cancelar(p.destino) || nxk_tarea_liberar(p.destino)) return 4;
    uint32_t antiguo=p.destino, nuevo;
    /* Reutilizar el slot no debe entregar el mensaje al nuevo destinatario. */
    if (nxk_tarea_crear(comprobar_vacio,0,&nuevo) || nuevo==antiguo || nxk_tareas_ejecutar(100) ||
        p.fase!=2 || p.retorno!=(uint32_t)-1 || errores || nxk_tarea_liberar(nuevo) ||
        nxk_tarea_liberar(p.emisor)) return 5;
    if (preparar_envio(&p,revisar_sin_envio,emitir_esperando) || nxk_tareas_ejecutar(1)!=1 ||
        p.fase!=1 || nxk_tarea_cancelar(p.emisor) || nxk_tareas_ejecutar(100) || p.fase!=1 ||
        p.fallos || nxk_tarea_liberar(p.destino) || nxk_tarea_liberar(p.emisor)) return 6;
    return 0;
}
extern int nxk_planificador_probar_saturacion(void);
extern int nxk_planificador_probar_resumen(void);
static int sin_eventos(const struct nxk_tarea_metricas *m) {
    return !m->cesiones && !m->dormidas && !m->esperas_mensaje && !m->esperas_envio &&
        !m->esperas_fin && !m->enviados && !m->recibidos && !m->plazos && !m->ciclos;
}
static void medir_eventos(void *argumento) {
    unsigned *fallos=argumento; struct nxk_tarea_metricas m; struct nxk_mensaje mensaje;
    uint32_t id=nxk_tarea_actual();
    if (nxk_tarea_metricas(id,&m) || m.activaciones!=1 || !sin_eventos(&m)) ++*fallos;
    nxk_tarea_ceder(); nxk_tarea_dormir(1);
    if (nxk_tarea_enviar(id,7) || nxk_tarea_esperar_mensaje(&mensaje,0) || mensaje.valor!=7 ||
        nxk_tarea_esperar_mensaje(&mensaje,1)!=1) ++*fallos;
    for (unsigned i=0;i<NXK_BUZON_MAX;++i) if (nxk_tarea_enviar(id,i)) ++*fallos;
    struct nxk_planificador_resumen r;
    if (nxk_planificador_consultar(&r) || r.registradas!=1 || r.pendientes!=1 ||
        r.finalizadas || r.mensajes!=4 || r.estados[NXK_TAREA_EJECUTANDO]!=1) ++*fallos;
    if (nxk_tarea_enviar_esperando(id,99,10)!=-4) ++*fallos;
    for (unsigned i=0;i<NXK_BUZON_MAX;++i) if (nxk_tarea_recibir(&mensaje) || mensaje.valor!=i) ++*fallos;
    if (nxk_tarea_metricas(id,&m) || m.activaciones!=4 || m.cesiones!=1 || m.dormidas!=1 ||
        m.esperas_mensaje!=1 || m.esperas_envio || m.esperas_fin || m.enviados!=5 ||
        m.recibidos!=5 || m.plazos!=1 || m.ciclos!=1) ++*fallos;
}
static int probar_metricas(void) {
    struct nxk_tarea_metricas m; uint32_t id,nuevo; unsigned fallos=0;
    if (nxk_planificador_probar_resumen() || nxk_planificador_probar_saturacion() || nxk_tarea_crear(medir_eventos,&fallos,&id) ||
        nxk_tarea_metricas(id,&m) || m.activaciones || !sin_eventos(&m) ||
        nxk_tarea_metricas(id,0)!=-1 || nxk_tareas_ejecutar(100) || fallos ||
        nxk_tarea_metricas(id,&m) || m.enviados!=5 || m.recibidos!=5 || nxk_tarea_liberar(id)) return 1;
    memset(&m,0xa5,sizeof(m));
    if (nxk_tarea_metricas(id,&m)!=-1) return 2;
    for (unsigned i=0;i<sizeof(m);++i) if (((unsigned char *)&m)[i]!=0xa5) return 2;
    if (nxk_tarea_crear(sumar,&fallos,&nuevo) || nuevo==id || nxk_tarea_metricas(nuevo,&m) ||
        m.activaciones || !sin_eventos(&m) || nxk_tarea_cancelar(nuevo) ||
        nxk_tarea_metricas(nuevo,&m) || m.activaciones || !sin_eventos(&m) ||
        nxk_tarea_liberar(nuevo)) return 3;
    return 0;
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
    if (probar_esperas()) return 19;
    if (probar_resultados() || probar_cadena()) return 20;
    if (probar_envio_esperando() || probar_flujo_multiple()) return 21;
    if (nxk_tareas_bloqueos_demo() || nxk_tareas_bloqueos_demo()) return 22;
    if (probar_metricas()) return 23;
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
int nxk_tareas_bloqueos_demo(void) { return -64; }
int nxk_tareas_resultados_demo(void) { return -64; }
int nxk_tareas_mensajes_demo(void) { return -64; }
int nxk_tareas_demo(void) { return -64; }
#endif
