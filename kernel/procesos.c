/* Planificacion CPL3 por rondas. La continuacion usa NEXTRY ASM; PCB en C.
 * Un ID de espacio obsoleto nunca hereda registros de la generacion anterior.
 * El PIT suspende procesos; el kernel y sus tareas conservan cooperacion. */
#include "procesos.h"
#include "paginas.h"
#include "paginacion.h"
#include "tareas.h"
#if defined(__i386__)
struct proceso {
    struct nxk_proceso_info info;
    uint32_t contexto[17];
    struct nxk_proceso_mensaje buzon[NXK_PROCESO_BUZON_MAX];
    uint32_t primero,buffer_mensaje,recepcion_pendiente;
};
static struct proceso procesos[NXK_ESPACIOS_MAX];
static unsigned cursor;
extern uint32_t nxk_reloj_ticks(void);
/* Plazos inferiores a media vuelta, sin comparar el valor absoluto con cero. */
static int vencido(uint32_t ahora,uint32_t plazo) { return ahora-plazo<0x80000000u; }
static int ocupado;
static void incrementar(uint32_t *n) { if (*n!=UINT32_MAX) ++*n; }
static int contexto_kernel(void) {
    uint32_t flags,cr0,cr3;
    __asm__ volatile("pushfl; popl %0":"=r"(flags));
    __asm__ volatile("mov %%cr0,%0":"=r"(cr0));
    __asm__ volatile("mov %%cr3,%0":"=r"(cr3));
    return (flags&0x200u) && (cr0&0x80000000u) && cr3==0x200000u && !nxk_tarea_actual();
}
/* Las otras API kernel pueden liberar un espacio inactivo; retirar entonces
 * el PCB antes de consultar, usarlo o asociar una generacion nueva. */
static void depurar(unsigned slot) {
    struct proceso *p=&procesos[slot]; struct nxk_espacio_info espacio;
    if (p->info.estado && nxk_espacio_consultar(p->info.id,&espacio)) memset(p,0,sizeof(*p));
    if (p->info.estado==NXK_PROCESO_DORMIDO && vencido(nxk_reloj_ticks(),p->info.despertar)) {
        p->info.estado=NXK_PROCESO_LISTO; p->info.despertar=0;
    }
    /* Una espera despertada conserva su buffer hasta ser despachada. Si el
     * mensaje llega despues del plazo y antes del turno, tiene prioridad. */
    if (p->info.estado==NXK_PROCESO_ESPERANDO_MENSAJE &&
        (p->info.mensajes || vencido(nxk_reloj_ticks(),p->info.despertar)))
        p->info.estado=NXK_PROCESO_LISTO;
}
static struct proceso *buscar(uint32_t id) {
    unsigned slot=(id&255u)-1u;
    if (slot>=NXK_ESPACIOS_MAX) return 0;
    depurar(slot); struct proceso *p=&procesos[slot];
    return p->info.estado && p->info.id==id ? p : 0;
}
static int asociar(uint32_t id) {
    unsigned slot=(id&255u)-1u;
    if (slot>=NXK_ESPACIOS_MAX) return -1;
    depurar(slot); struct proceso *p=&procesos[slot];
    if (p->info.estado) return -2;
    memset(p,0,sizeof(*p)); p->info.id=id; p->info.estado=NXK_PROCESO_LISTO;
    for (unsigned i=0;i<4;++i) p->contexto[i]=51;
    p->contexto[12]=NXK_ESPACIO_CODIGO; p->contexto[13]=43;
    p->contexto[14]=0x202; p->contexto[15]=NXK_ESPACIO_PILA+4096; p->contexto[16]=51;
    return 0;
}
int nxk_proceso_crear(const void *codigo,size_t n,uint32_t *id) {
    if (!id || !codigo || !n || n>4096) return -1;
    if (ocupado || !contexto_kernel()) return -3;
    uint32_t nuevo; int r=nxk_espacio_crear(codigo,n,&nuevo); if (r) return r;
    r=asociar(nuevo);
    if (r) { nxk_espacio_destruir(nuevo); return r; }
    *id=nuevo; return 0;
}
int nxk_proceso_cargar_archivo(const char *nombre,uint32_t *id) {
    if (!nombre || !id) return -1;
    if (ocupado || !contexto_kernel()) return -3;
    uint32_t nuevo; int r=nxk_usuario_preparar_archivo(nombre,&nuevo); if (r) return r;
    r=asociar(nuevo);
    if (r) { nxk_espacio_destruir(nuevo); return r; }
    *id=nuevo; return 0;
}
int nxk_proceso_consultar(uint32_t id,struct nxk_proceso_info *salida) {
    struct proceso *p=buscar(id); if (!p || !salida) return -1;
    *salida=p->info; return 0;
}
int nxk_proceso_enumerar(unsigned slot,struct nxk_proceso_info *salida) {
    if (slot>=NXK_ESPACIOS_MAX || !salida) return -1;
    depurar(slot); if (!procesos[slot].info.estado) return -2;
    *salida=procesos[slot].info; return 0;
}
int nxk_proceso_gestionado(uint32_t id) { return buscar(id)!=0; }
static int terminal(const struct proceso *p) {
    return p->info.estado==NXK_PROCESO_TERMINADO || p->info.estado==NXK_PROCESO_FALLIDO ||
        p->info.estado==NXK_PROCESO_CANCELADO;
}
static void limpiar_espera(struct proceso *p) {
    p->buffer_mensaje=0; p->recepcion_pendiente=0; p->info.despertar=0;
}
static void retirar_mensaje(struct proceso *p) {
    memset(&p->buzon[p->primero],0,sizeof(p->buzon[0]));
    p->primero=(p->primero+1)%NXK_PROCESO_BUZON_MAX;
    --p->info.mensajes; incrementar(&p->info.recibidos);
}
static int enviar(struct proceso *destino,uint32_t origen,uint32_t valor) {
    if (!destino) return -1;
    if (terminal(destino)) return -3;
    if (destino->info.mensajes==NXK_PROCESO_BUZON_MAX) return -2;
    unsigned i=(destino->primero+destino->info.mensajes)%NXK_PROCESO_BUZON_MAX;
    destino->buzon[i]=(struct nxk_proceso_mensaje){origen,valor};
    ++destino->info.mensajes;
    if (destino->info.estado==NXK_PROCESO_ESPERANDO_MENSAJE)
        destino->info.estado=NXK_PROCESO_LISTO;
    return 0;
}
int nxk_proceso_enviar(uint32_t destino,uint32_t valor) {
    if (ocupado || !contexto_kernel()) return -3;
    return enviar(buscar(destino),0,valor);
}
int nxk_proceso_enviar_desde(uint32_t origen,uint32_t destino,uint32_t valor) {
    struct proceso *p=buscar(origen);
    if (!ocupado || !p || p->info.estado!=NXK_PROCESO_EJECUTANDO) return -1;
    int r=enviar(buscar(destino),origen,valor);
    if (!r) incrementar(&p->info.enviados);
    return r;
}
int nxk_proceso_recibir_actual(uint32_t id,struct nxk_proceso_mensaje *salida) {
    struct proceso *p=buscar(id);
    if (!ocupado || !p || !salida || p->info.estado!=NXK_PROCESO_EJECUTANDO) return -1;
    if (!p->info.mensajes) return -2;
    *salida=p->buzon[p->primero]; retirar_mensaje(p); return 0;
}
int nxk_proceso_preparar_espera(uint32_t id,uint32_t buffer_virtual) {
    struct proceso *p=buscar(id);
    if (!ocupado || !p || p->info.estado!=NXK_PROCESO_EJECUTANDO || p->recepcion_pendiente ||
        buffer_virtual<NXK_ESPACIO_PILA || buffer_virtual>NXK_ESPACIO_PILA+4096-sizeof(struct nxk_proceso_mensaje))
        return -1;
    p->buffer_mensaje=buffer_virtual; p->recepcion_pendiente=1; return 0;
}
int nxk_proceso_cancelar(uint32_t id) {
    if (ocupado || !contexto_kernel()) return -3;
    struct proceso *p=buscar(id); if (!p) return -1;
    if (p->info.estado!=NXK_PROCESO_LISTO && p->info.estado!=NXK_PROCESO_DORMIDO &&
        p->info.estado!=NXK_PROCESO_ESPERANDO_MENSAJE) return -2;
    limpiar_espera(p); memset(p->buzon,0,sizeof(p->buzon)); p->primero=0; p->info.mensajes=0;
    p->info.estado=NXK_PROCESO_CANCELADO;
    memset(&p->info.ultimo,0,sizeof(p->info.ultimo)); p->info.ultimo.salida=UINT32_MAX;
    return 0;
}
static int ejecutar_uno(struct proceso *p) {
    if (p->info.estado!=NXK_PROCESO_LISTO) return -2;
    if (p->recepcion_pendiente) {
        if (p->info.mensajes) {
            /* CR3 kernel: validar y copiar antes de consumir. El motor nunca
             * recibe un buffer kernel ni una traduccion virtual prestada. */
            int r=nxk_espacio_escribir(p->info.id,p->buffer_mensaje,&p->buzon[p->primero],sizeof(p->buzon[0]));
            if (r) {
                limpiar_espera(p); p->info.error_motor=(uint32_t)(-r);
                p->info.estado=NXK_PROCESO_FALLIDO; return r;
            }
            retirar_mensaje(p); p->contexto[11]=0;
        } else { p->contexto[11]=1; incrementar(&p->info.plazos); }
        limpiar_espera(p);
    }
    p->info.estado=NXK_PROCESO_EJECUTANDO; p->info.despertar=0; incrementar(&p->info.turnos);
    uint32_t cedido=0,despertar=0; struct nxk_usuario_resultado resultado;
    int r=nxk_usuario_reanudar_espacio(p->info.id,p->contexto,&resultado,&cedido,&despertar);
    if (r) {
        limpiar_espera(p); p->info.error_motor=(uint32_t)(-r);
        p->info.estado=NXK_PROCESO_FALLIDO; return r;
    }
    p->info.ultimo=resultado;
    if (resultado.fallo) { limpiar_espera(p); p->info.estado=NXK_PROCESO_FALLIDO; }
    else if (cedido==NXK_USUARIO_MENSAJE) {
        p->info.estado=NXK_PROCESO_ESPERANDO_MENSAJE; p->info.despertar=despertar;
        incrementar(&p->info.esperas_mensaje);
    }
    else if (cedido==NXK_USUARIO_DORMIR) {
        p->info.estado=NXK_PROCESO_DORMIDO; p->info.despertar=despertar;
        incrementar(&p->info.dormidas);
    }
    else if (cedido) {
        p->info.estado=NXK_PROCESO_LISTO;
        if (cedido==NXK_USUARIO_QUANTUM) incrementar(&p->info.preempciones);
        else incrementar(&p->info.cesiones);
    }
    else { limpiar_espera(p); p->info.estado=NXK_PROCESO_TERMINADO; }
    return 0;
}
int nxk_proceso_paso(uint32_t id) {
    if (ocupado || !contexto_kernel()) return -3;
    struct proceso *p=buscar(id); if (!p) return -1;
    if (p->info.estado!=NXK_PROCESO_LISTO) return -2;
    ocupado=1; int r=ejecutar_uno(p); ocupado=0; return r;
}
int nxk_procesos_ejecutar(uint32_t presupuesto) {
    if (!presupuesto || presupuesto>1024) return -1;
    if (ocupado || !contexto_kernel()) return -3;
    ocupado=1; uint32_t usados=0; int r=0;
    for (;;) {
        unsigned elegida=NXK_ESPACIOS_MAX,esperando=0;
        for (unsigned n=0;n<NXK_ESPACIOS_MAX;++n) {
            unsigned i=(cursor+n)%NXK_ESPACIOS_MAX; depurar(i);
            if (procesos[i].info.estado==NXK_PROCESO_DORMIDO ||
                procesos[i].info.estado==NXK_PROCESO_ESPERANDO_MENSAJE) ++esperando;
            if (procesos[i].info.estado==NXK_PROCESO_LISTO) { elegida=i; break; }
        }
        if (elegida==NXK_ESPACIOS_MAX) { r=esperando ? 2 : 0; break; }
        if (usados==presupuesto) { r=1; break; }
        cursor=(elegida+1)%NXK_ESPACIOS_MAX; ++usados;
        r=ejecutar_uno(&procesos[elegida]); if (r) break;
    }
    ocupado=0; return r;
}
int nxk_proceso_liberar(uint32_t id) {
    if (ocupado || !contexto_kernel()) return -3;
    struct proceso *p=buscar(id); if (!p) return -1;
    if (p->info.estado==NXK_PROCESO_EJECUTANDO) return -2;
    int r=nxk_espacio_destruir(id); if (r) return r;
    memset(p,0,sizeof(*p)); return 0;
}
/* Dos programas conservan EBX y ESP durante dos cesiones. Tras la primera
 * cesion verifican EAX=0; la segunda conserva un valor apilado por cada uno. */
int nxk_procesos_demo(void) {
    if (ocupado || !contexto_kernel()) return -3;
    /* La demo no puede avanzar procesos ajenos mediante el ejecutor global. */
    for (unsigned i=0;i<NXK_ESPACIOS_MAX;++i) {
        depurar(i); if (procesos[i].info.estado) return -2;
    }
    unsigned char codigo[]={0xbb,0x10,0x11,0x11,0x11,0xb8,9,0,0,0,0xcd,0x81,
        0x85,0xc0,0x74,2,0x0f,0x0b,0x43,0x53,0xb8,9,0,0,0,0xcd,0x81,
        0x5b,0x43,0x89,0xd8,0xcd,0x80};
    uint32_t ids[2]={0}, libres=nxk_paginas_libres(), guardas=nxk_paginacion_guardas();
    int fallo=0;
    if (nxk_proceso_crear(codigo,sizeof(codigo),&ids[0])) fallo=1;
    codigo[1]=0x20; codigo[2]=codigo[3]=codigo[4]=0x22;
    if (!fallo && nxk_proceso_crear(codigo,sizeof(codigo),&ids[1])) fallo=2;
    for (unsigned etapa=0;!fallo && etapa<3;++etapa) {
        int esperado=etapa<2 ? 1 : 0;
        if (nxk_procesos_ejecutar(2)!=esperado) { fallo=3; break; }
        for (unsigned i=0;i<2;++i) {
            struct nxk_proceso_info info;
            if (nxk_proceso_consultar(ids[i],&info) || info.turnos!=etapa+1 ||
                info.cesiones!=(etapa<2 ? etapa+1 : 2) ||
                info.estado!=(etapa<2 ? NXK_PROCESO_LISTO : NXK_PROCESO_TERMINADO) ||
                (etapa==2 && (info.ultimo.fallo || info.ultimo.salida!=(i ? 0x22222222u : 0x11111112u)))) { fallo=4; break; }
        }
    }
    for (unsigned i=0;i<2;++i) if (ids[i] && nxk_proceso_liberar(ids[i])) fallo=5;
    if (nxk_paginas_libres()!=libres || nxk_paginacion_guardas()!=guardas) fallo=6;
    return fallo ? -fallo : 0;
}
#ifdef NXK_TEST
static int intacto(const struct nxk_proceso_info *p) {
    for (unsigned i=0;i<sizeof(*p);++i) if (((const unsigned char *)p)[i]!=0xa5) return 0;
    return 1;
}
int nxk_procesos_probar(void) {
    if (!contexto_kernel()) return 1;
    if (!vencido(3,0xfffffffeu) || vencido(0xfffffffdu,3) || !vencido(0,0) ||
        vencido(0xffffffffu,0) || !vencido(7,7)) return 13;
    uint32_t libres=nxk_paginas_libres(), ids[4]={0}, salida=0xdeadbeef;
    struct nxk_proceso_info info; memset(&info,0xa5,sizeof(info));
    if (nxk_proceso_crear(0,1,&salida)!=-1 || nxk_proceso_crear("x",0,&salida)!=-1 ||
        nxk_proceso_crear("x",4097,&salida)!=-1 || nxk_proceso_crear("x",1,0)!=-1 ||
        nxk_proceso_consultar(0,&info)!=-1 || !intacto(&info) || salida!=0xdeadbeef ||
        nxk_proceso_enumerar(4,&info)!=-1 || nxk_procesos_ejecutar(0)!=-1 ||
        nxk_procesos_ejecutar(1025)!=-1) return 2;
    static const unsigned char salir[]={0xb8,42,0,0,0,0xcd,0x80};
    for (unsigned i=0;i<4;++i) if (nxk_proceso_crear(salir,sizeof(salir),&ids[i])) return 3;
    if (nxk_paginas_libres()!=libres-20 || nxk_proceso_crear(salir,sizeof(salir),&salida)!=-2 || salida!=0xdeadbeef) return 4;
    for (unsigned i=0;i<4;++i) if (!nxk_proceso_gestionado(ids[i]) ||
        nxk_proceso_enumerar(i,&info) || info.id!=ids[i] || info.estado!=NXK_PROCESO_LISTO) return 5;
    if (nxk_procesos_ejecutar(8)) return 6;
    for (unsigned i=0;i<4;++i) {
        if (nxk_proceso_consultar(ids[i],&info) || info.estado!=NXK_PROCESO_TERMINADO ||
            info.turnos!=1 || info.cesiones || info.ultimo.fallo || info.ultimo.salida!=42 || nxk_proceso_paso(ids[i])!=-2 ||
            nxk_proceso_liberar(ids[i])) return 7;
    }
    memset(&info,0xa5,sizeof(info));
    if (nxk_proceso_consultar(ids[0],&info)!=-1 || nxk_proceso_gestionado(ids[0]) ||
        nxk_proceso_enumerar(0,&info)!=-2 || !intacto(&info)) return 8;
    /* Liberacion por API de espacio: no conservar PCB ni usar su ID viejo. */
    uint32_t viejo=ids[0];
    if (nxk_proceso_crear(salir,sizeof(salir),&ids[0]) || ids[0]==viejo ||
        nxk_espacio_destruir(ids[0]) || nxk_proceso_gestionado(ids[0]) ||
        nxk_proceso_enumerar(0,&info)!=-2 || nxk_proceso_liberar(ids[0])!=-1) return 9;
    static const unsigned char malo[]={0x0f,0x0b};
    if (nxk_proceso_crear(malo,sizeof(malo),&ids[0]) || nxk_proceso_crear(salir,sizeof(salir),&ids[1]) ||
        nxk_procesos_ejecutar(8) || nxk_proceso_consultar(ids[0],&info) ||
        info.estado!=NXK_PROCESO_FALLIDO || !info.ultimo.fallo || info.ultimo.vector!=6 ||
        nxk_proceso_consultar(ids[1],&info) || info.estado!=NXK_PROCESO_TERMINADO ||
        info.ultimo.salida!=42 || nxk_proceso_enviar(ids[0],1)!=-3 ||
        nxk_proceso_enviar(ids[1],1)!=-3 || nxk_proceso_cancelar(ids[0])!=-2 ||
        nxk_proceso_cancelar(ids[1])!=-2 ||
        nxk_proceso_liberar(ids[0]) || nxk_proceso_liberar(ids[1])) return 10;
    /* El envio kernel no inventa identidad de usuario. Cola llena y API
     * interna fuera de un turno conservan el contenido y la salida. */
    if (nxk_proceso_crear(salir,sizeof(salir),&ids[0])) return 14;
    struct nxk_proceso_mensaje mensaje={0xdeadbeefu,0xcafebabeu};
    if (nxk_proceso_enviar(0,1)!=-1 || nxk_proceso_enviar_desde(ids[0],ids[0],1)!=-1 ||
        nxk_proceso_recibir_actual(ids[0],&mensaje)!=-1 ||
        mensaje.origen!=0xdeadbeefu || mensaje.valor!=0xcafebabeu ||
        nxk_proceso_preparar_espera(ids[0],NXK_ESPACIO_PILA)!=-1) return 15;
    for (unsigned i=0;i<NXK_PROCESO_BUZON_MAX;++i)
        if (nxk_proceso_enviar(ids[0],i)) return 16;
    if (nxk_proceso_enviar(ids[0],4)!=-2 || nxk_proceso_consultar(ids[0],&info) ||
        info.mensajes!=4 || info.enviados || info.recibidos || info.esperas_mensaje || info.plazos ||
        nxk_proceso_cancelar(ids[0]) || !nxk_proceso_gestionado(ids[0]) ||
        nxk_proceso_consultar(ids[0],&info) || info.estado!=NXK_PROCESO_CANCELADO ||
        info.mensajes || info.despertar || info.ultimo.fallo || info.ultimo.salida!=UINT32_MAX ||
        info.turnos || nxk_proceso_paso(ids[0])!=-2 || nxk_proceso_cancelar(ids[0])!=-2 ||
        nxk_proceso_enviar(ids[0],1)!=-3 || nxk_procesos_ejecutar(1) ||
        nxk_proceso_liberar(ids[0]) || nxk_proceso_enviar(ids[0],1)!=-1 ||
        nxk_proceso_cancelar(ids[0])!=-1 || nxk_paginas_libres()!=libres) return 17;
    /* Cancelar tambien retira un plazo de dormir sin liberar sus paginas ni
     * convertir el dormido en listo. No puede reanudar por una ronda posterior. */
    static const unsigned char dormir[]={0xbb,0xff,0xff,0xff,0x7f,0xb8,10,0,0,0,0xcd,0x81,0xcd,0x80};
    if (nxk_proceso_crear(dormir,sizeof(dormir),&ids[0]) || nxk_proceso_paso(ids[0]) ||
        nxk_proceso_consultar(ids[0],&info) || info.estado!=NXK_PROCESO_DORMIDO || info.dormidas!=1 ||
        nxk_proceso_cancelar(ids[0]) || nxk_proceso_consultar(ids[0],&info) ||
        info.estado!=NXK_PROCESO_CANCELADO || info.despertar || info.turnos!=1 ||
        nxk_paginas_libres()!=libres-5 || nxk_procesos_ejecutar(4) ||
        nxk_proceso_liberar(ids[0]) || nxk_paginas_libres()!=libres) return 18;
    if (nxk_procesos_demo()) return 11;
    size_t heap=nxk_heap_used();
    if (nxk_procesos_demo() || nxk_procesos_demo() || nxk_heap_used()!=heap || nxk_paginas_libres()!=libres) return 12;
    return 0;
}
#endif
#else
int nxk_proceso_crear(const void *c,size_t n,uint32_t *id) { (void)c;(void)n;(void)id;return -64; }
int nxk_proceso_cargar_archivo(const char *n,uint32_t *id) { (void)n;(void)id;return -64; }
int nxk_proceso_consultar(uint32_t id,struct nxk_proceso_info *s) { (void)id;(void)s;return -64; }
int nxk_proceso_enumerar(unsigned i,struct nxk_proceso_info *s) { (void)i;(void)s;return -64; }
int nxk_proceso_paso(uint32_t id) { (void)id;return -64; }
int nxk_proceso_enviar(uint32_t id,uint32_t valor) { (void)id;(void)valor;return -64; }
int nxk_proceso_enviar_desde(uint32_t o,uint32_t d,uint32_t v) { (void)o;(void)d;(void)v;return -64; }
int nxk_proceso_recibir_actual(uint32_t id,struct nxk_proceso_mensaje *s) { (void)id;(void)s;return -64; }
int nxk_proceso_preparar_espera(uint32_t id,uint32_t b) { (void)id;(void)b;return -64; }
int nxk_proceso_cancelar(uint32_t id) { (void)id;return -64; }
int nxk_procesos_ejecutar(uint32_t n) { (void)n;return -64; }
int nxk_proceso_liberar(uint32_t id) { (void)id;return -64; }
int nxk_proceso_gestionado(uint32_t id) { (void)id;return 0; }
int nxk_procesos_demo(void) { return -64; }
#endif
