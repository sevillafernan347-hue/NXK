/* GUIA DE LECTURA NXK - kernel/excepciones.c
 * Construye la IDT: tabla que indica a la CPU donde ir ante una excepcion. Solo implementada para x86.
 * Contexto y orden de estudio: ver LEER-CODIGO.md en la carpeta principal.
 */
#include <stdint.h>
#include "nxk_es.h"
#include "nxk.h"
#include "usuario.h"
#include "archivos.h"
#include "volumen.h"
#include "espacios.h"
#include "tareas.h"
#include "procesos.h"
#if defined(__i386__)
struct __attribute__((packed)) puerta {
    uint16_t bajo, selector;
    uint8_t cero, atributos;
    uint16_t alto;
};
struct __attribute__((packed)) descriptor { uint16_t limite; uint32_t base; };
static struct puerta tabla[256];
extern const unsigned char excepcion_nxa_inicio[], excepcion_nxa_fin[];
extern void nxk_excepcion_fatal(const uint32_t *pila);
static unsigned char entrada[33][48];
/* Plantilla generada por NEXTRY ASM; el puente resuelve la llamada C. */
extern const unsigned char reloj_nxa_inicio[], reloj_nxa_fin[];
static unsigned char reloj_entrada[42];
static volatile uint32_t ticks;
static int reloj_listo, emergencia_lista;
static void puerto(uint16_t p,uint8_t v) {
    __asm__ volatile("outb %0,%1" :: "a"(v),"Nd"(p));
    __asm__ volatile("outb %%al,$0x80" :: "a"(0));
}
static void usuario_temporizador(const uint32_t *contexto);
void nxk_reloj_irq(const uint32_t *contexto) {
    ++ticks; puerto(0x20,0x20); /* EOI antes de una posible salida anticipada. */
    usuario_temporizador(contexto);
}
uint32_t nxk_reloj_ticks(void) { return ticks; }
void nxk_reloj_iniciar(void) {
    __asm__ volatile("cli" ::: "memory");
    if ((uintptr_t)reloj_nxa_fin-(uintptr_t)reloj_nxa_inicio!=42 ||
        reloj_nxa_inicio[25]!=0xb8 || reloj_nxa_inicio[41]!=0xcf)
        nxk_es_detener();
    for (unsigned i=0;i<42;++i) reloj_entrada[i]=reloj_nxa_inicio[i];
    uintptr_t f=(uintptr_t)nxk_reloj_irq, e=(uintptr_t)reloj_entrada;
    for (unsigned i=0;i<4;++i) reloj_entrada[26+i]=(unsigned char)(f>>(8*i));
    uint16_t cs; __asm__ volatile("mov %%cs,%0":"=r"(cs));
    tabla[32]=(struct puerta){(uint16_t)e,cs,0,0x8e,(uint16_t)(e>>16)};
    /* PIC maestro/esclavo a 32/40; solo IRQ0 habilitada. */
    puerto(0x21,0xff); puerto(0xa1,0xff);
    puerto(0x20,0x11); puerto(0xa0,0x11);
    puerto(0x21,32); puerto(0xa1,40);
    puerto(0x21,4); puerto(0xa1,2);
    puerto(0x21,1); puerto(0xa1,1);
    puerto(0x21,0xfe); puerto(0xa1,0xff);
    /* PIT canal 0: divisor 11932, aproximadamente 100 Hz. */
    puerto(0x43,0x36); puerto(0x40,0x9c); puerto(0x40,0x2e);
    ticks=0; reloj_listo=1;
    __asm__ volatile("sti" ::: "memory");
}
/* Doble fallo: cambio de tarea hardware a TSS con pila independiente.
 * Solo fatal, una CPU. No retorna ni se usa como planificador. */
static uint64_t gdt_emergencia[7];
static uint32_t tss_normal[26], tss_emergencia[26];
static void descriptor_tss(unsigned indice,void *tss) {
    uintptr_t base=(uintptr_t)tss;
    unsigned char *d=(void *)&gdt_emergencia[indice];
    d[0]=103; d[1]=0; d[2]=(unsigned char)base; d[3]=(unsigned char)(base>>8);
    d[4]=(unsigned char)(base>>16); d[5]=0x89; d[6]=0; d[7]=(unsigned char)(base>>24);
}
static _Noreturn void doble_fallo(void) {
    nxk_es_mostrar("NXK: DOBLE FALLO - PILA DE EMERGENCIA ACTIVA");
    nxk_es_mostrar("ESP anterior:"); nxk_es_numero(tss_normal[14]);
#ifdef NXK_EXCEPTION_TEST
    nxk_es_escribir_puerto(0xf4,0x12);
#endif
    nxk_es_detener();
}
int nxk_emergencia_iniciar(void) {
    struct descriptor anterior;
    __asm__ volatile("sgdt %0":"=m"(anterior));
    /* Contrato BIOS actual: nulo, codigo 8 y datos 16. No alterar otros GDT. */
    if (anterior.limite!=23) return -1;
    unsigned char *pila=nxk_alloc(4096);
    if (!pila) return -2;
    for (unsigned i=0;i<24;++i) ((unsigned char *)gdt_emergencia)[i]=((const unsigned char *)(uintptr_t)anterior.base)[i];
    gdt_emergencia[5]=0x00cffa000000ffffULL; /* CS usuario: selector 43. */
    gdt_emergencia[6]=0x00cff2000000ffffULL; /* DS usuario: selector 51. */
    descriptor_tss(3,tss_normal); descriptor_tss(4,tss_emergencia);
    tss_normal[25]=104u<<16; tss_emergencia[25]=104u<<16;
    __asm__ volatile("mov %%cr3,%0":"=r"(tss_emergencia[7]));
    tss_emergencia[8]=(uint32_t)(uintptr_t)doble_fallo;
    tss_emergencia[9]=2; /* IF apagada durante diagnostico fatal. */
    tss_emergencia[14]=(uint32_t)(uintptr_t)(pila+4096);
    tss_emergencia[18]=16; tss_emergencia[19]=8;
    tss_emergencia[20]=16; tss_emergencia[21]=16;
    tss_emergencia[22]=16; tss_emergencia[23]=16;
    struct descriptor nueva={sizeof(gdt_emergencia)-1,(uint32_t)(uintptr_t)gdt_emergencia};
    __asm__ volatile("lgdt %0" :: "m"(nueva) : "memory");
    uint16_t selector=24;
    __asm__ volatile("ltr %0" :: "r"(selector) : "memory");
    tabla[8]=(struct puerta){0,32,0,0x85,0}; /* Task gate, DPL0. */
    emergencia_lista=1;
    return 0;
}
/* Prueba aislada: dos paginas de usuario, resto de PTE solo supervisor.
 * INT 0x80 demuestra la entrada al kernel; el manejador fatal termina la prueba.
 * No es todavia una ABI publica de llamadas al sistema. */
_Noreturn void nxk_usuario_probar(void) {
    unsigned char *reserva=nxk_alloc(3*4096), *pila_kernel=nxk_alloc(4096);
    if (!reserva || !pila_kernel) nxk_es_detener();
    uintptr_t codigo=((uintptr_t)reserva+4095)&~(uintptr_t)4095;
    uintptr_t pila=codigo+4096;
    unsigned char *bytes=(void *)codigo;
#if defined(NXK_EXCEPTION_VECTOR) && NXK_EXCEPTION_VECTOR == 514
    bytes[0]=0xfa; /* CLI prohibida con CPL3 e IOPL0. */
#elif defined(NXK_EXCEPTION_VECTOR) && NXK_EXCEPTION_VECTOR == 614
    /* MOV [0x10000], 1: intento de escribir en codigo supervisor. */
    const unsigned char intento[]={0xc7,0x05,0,0,1,0,1,0,0,0};
    memcpy(bytes,intento,sizeof(intento));
#else
    bytes[0]=0xcd; bytes[1]=0x80; bytes[2]=0x0f; bytes[3]=0x0b;
#endif
    tss_normal[1]=(uint32_t)(uintptr_t)(pila_kernel+4096);
    tss_normal[2]=16;
    uintptr_t puerta_usuario=(uintptr_t)entrada[32];
    tabla[128]=(struct puerta){(uint16_t)puerta_usuario,8,0,0xee,(uint16_t)(puerta_usuario>>16)};
    volatile uint32_t *paginas=(void *)0x201000, *directorio=(void *)0x200000;
    paginas[codigo/4096]=(uint32_t)codigo|5; /* Usuario, solo lectura. */
    paginas[pila/4096]=(uint32_t)pila|7;
    directorio[codigo>>22]|=4;
    __asm__ volatile("mov %0,%%cr3" :: "r"(0x200000u) : "memory");
    /* IF apagada en la prueba: IRQ0 aun no normaliza segmentos de usuario. */
    __asm__ volatile("mov $51,%%ax; mov %%ax,%%ds; mov %%ax,%%es;"
        "pushl $51; pushl %0; pushl $2; pushl $43; pushl %1; iret"
        :: "r"((uint32_t)(pila+4096)),"r"((uint32_t)codigo) : "eax","memory");
    __builtin_unreachable();
}
/* Demostracion reutilizable: programa conocido que termina con EAX=42. */
uint32_t nxk_usuario_pila_guardada, nxk_usuario_cs_guardado;
extern uint32_t nxk_usuario_entrar(uint32_t codigo,uint32_t pila);
extern void nxk_usuario_salir(void);
static int usuario_activo;
static uint32_t usuario_inicio,usuario_id_activo;
static uintptr_t usuario_base;
static const unsigned char *usuario_carga;
static size_t usuario_carga_longitud;
extern const unsigned char servicio_nxa_inicio[], servicio_nxa_fin[];
static unsigned char servicio_entrada[42];
static uint32_t *contexto_reanudable;
static uint32_t motivo_retorno,plazo_retorno;
extern uint32_t nxk_usuario_reanudar(const uint32_t *marco);
extern _Noreturn void nxk_usuario_abortar(uint32_t resultado);
extern void nxk_pe_salida(const char *datos,uint32_t n);
static int usuario_rango(uint32_t p,uint32_t n,int escritura) {
    if (!n) return 1;
    /* El proceso puede usar sus paginas de datos reservadas. El validador
     * comprueba todo el rango y todos sus respaldos antes de acceder en ring0.
     * Las rutas antiguas mantienen el contrato de dos paginas contiguas. */
    if (contexto_reanudable)
        return !nxk_espacio_validar_rango(usuario_id_activo,p,n,escritura);
    uintptr_t minimo=usuario_base+(escritura ? 4096 : 0);
    return p>=minimo && p-usuario_base<8192 && n<=8192-(p-usuario_base);
}
/* El nombre ya es una copia kernel terminada. El filtro no consulta el FS,
 * por lo que una denegacion no revela existencia ni modifica archivos. */
static int usuario_archivo_autorizado(uint32_t *c,const char *nombre) {
    if (!contexto_reanudable) return 1;
    int r=nxk_proceso_archivo_permitido(usuario_id_activo,nombre);
    if (r==1) return 1;
    c[11]=r==0 ? (uint32_t)-5 : UINT32_MAX; return 0;
}
static void usuario_servicio(uint32_t *c) {
    uint32_t servicio=c[11], p=c[8], n=c[10];
    c[11]=0xffffffffu;
    if (!usuario_activo || (c[13]&3)!=3) return;
    /* Permisos por proceso antes de leer nombres, copiar buffers o modificar
     * archivos. Las rutas antiguas sin PCB conservan su contrato anterior. */
    uint32_t requerido=0;
    if (servicio==1) requerido=NXK_PERM_SALIDA;
    else if (servicio==3 || servicio==5) requerido=NXK_PERM_LEER_ARCHIVOS;
    else if (servicio==8) requerido=NXK_PERM_LEER_ARCHIVOS|NXK_PERM_MODIFICAR_ARCHIVOS;
    else if (servicio==2 || servicio==4 || servicio==6 || servicio==7) requerido=NXK_PERM_MODIFICAR_ARCHIVOS;
    if (contexto_reanudable && requerido) {
        uint32_t permisos;
        if (nxk_proceso_permisos(usuario_id_activo,&permisos) || (permisos&requerido)!=requerido) {
            c[11]=(uint32_t)-5; return;
        }
    }
    char datos[256];
    /* Solo los procesos reanudables habilitan la cesion; ABI anterior rechaza9. */
    if (servicio==9 && contexto_reanudable) {
        c[11]=0; memcpy(contexto_reanudable,c,17*sizeof(uint32_t));
        motivo_retorno=NXK_USUARIO_CEDER; nxk_usuario_cs_guardado=c[13]; nxk_usuario_abortar(0);
    }
    if (servicio==10 && contexto_reanudable) {
        if (p>0x7fffffffu) return; /* EAX=-1, ningun estado ni marco modificado. */
        c[11]=0; memcpy(contexto_reanudable,c,17*sizeof(uint32_t));
        motivo_retorno=p ? NXK_USUARIO_DORMIR : NXK_USUARIO_CEDER;
        plazo_retorno=p ? ticks+p : 0;
        nxk_usuario_cs_guardado=c[13]; nxk_usuario_abortar(0);
    }
    /* IPC solo de procesos gestionados. El emisor procede del turno activo,
     * nunca de un ID proporcionado por la aplicacion. Una CPU, IRQ sin IPC. */
    if (servicio==11 && contexto_reanudable) {
        c[11]=(uint32_t)nxk_proceso_enviar_desde(usuario_id_activo,p,n); return;
    }
    if (servicio==12 && contexto_reanudable) {
        uint32_t espera=c[9];
        if (espera>0x7fffffffu || !usuario_rango(p,8,1)) return;
        struct nxk_proceso_mensaje mensaje;
        int r=nxk_proceso_recibir_actual(usuario_id_activo,&mensaje);
        if (!r) {
            /* Rango completo validado antes de extraer; CR3 sigue siendo el
             * privado de este proceso, por eso la copia es directa. */
            memcpy((void *)(uintptr_t)p,&mensaje,sizeof(mensaje)); c[11]=0; return;
        }
        if (r!=-2) return;
        c[11]=1; /* Consulta vacia o vencimiento: buffer sin modificar. */
        if (!espera) return;
        if (nxk_proceso_preparar_espera(usuario_id_activo,p)) { c[11]=UINT32_MAX; return; }
        memcpy(contexto_reanudable,c,17*sizeof(uint32_t));
        motivo_retorno=NXK_USUARIO_MENSAJE; plazo_retorno=ticks+espera;
        nxk_usuario_cs_guardado=c[13]; nxk_usuario_abortar(0);
    }
    if (servicio==13 && contexto_reanudable) { c[11]=usuario_id_activo; return; }
    if (servicio==16 && contexto_reanudable) {
        c[11]=(uint32_t)nxk_proceso_restringir_actual(usuario_id_activo,p); return;
    }
    if (servicio==17 && contexto_reanudable) {
        uint32_t permisos;
        if (!nxk_proceso_permisos(usuario_id_activo,&permisos)) c[11]=permisos;
        return;
    }
    /* Reserva acotada al espacio que ejecuta: nunca acepta un ID de la app.
     * El cupo evita que un proceso consuma todas las paginas del sistema.
     * Los buffers usan solo regiones propias validadas por usuario_rango. */
    if (servicio==18 && contexto_reanudable) {
        uint32_t direccion;
        c[11]=nxk_espacio_reservar_pagina(usuario_id_activo,&direccion) ? 0 : direccion;
        return;
    }
    if (servicio==19 && contexto_reanudable) {
        c[11]=nxk_espacio_liberar_pagina(usuario_id_activo,p) ? UINT32_MAX : 0;
        return;
    }
    if (servicio==20 && contexto_reanudable) {
        if (n>NXK_PROCESO_ARCHIVOS_MAX) return;
        char nombres[NXK_PROCESO_ARCHIVOS_MAX][24];
        if (n) {
            /* Validacion completa antes de copiar: un hueco no aplica una
             * lista parcial. Cantidad0 no desreferencia el puntero. */
            if (!usuario_rango(p,n*24u,0)) return;
            memcpy(nombres,(const void *)(uintptr_t)p,n*24u);
        }
        c[11]=(uint32_t)nxk_proceso_limitar_archivos_actual(usuario_id_activo,n ? nombres : 0,n);
        return;
    }
    if ((servicio==14 || servicio==15) && contexto_reanudable) {
        uint32_t espera=c[9];
        if (espera>0x7fffffffu || (servicio==15 && !usuario_rango(n,8,1))) return;
        struct nxk_proceso_resultado resultado;
        int r=servicio==14 ? nxk_proceso_enviar_esperando_actual(usuario_id_activo,p,n,espera) :
            nxk_proceso_esperar_fin_actual(usuario_id_activo,p,n,espera,&resultado);
        if (r==0 && servicio==15)
            memcpy((void *)(uintptr_t)n,&resultado,sizeof(resultado));
        c[11]=r==2 ? 1u : (uint32_t)r;
        if (r!=2) return;
        /* El gestor guarda destino/valor o buffer; el marco conserva todos
         * los registros, incluido EDX. El plazo empieza en esta peticion. */
        memcpy(contexto_reanudable,c,17*sizeof(uint32_t));
        motivo_retorno=servicio==14 ? NXK_USUARIO_ENVIO : NXK_USUARIO_FIN;
        plazo_retorno=ticks+espera;
        nxk_usuario_cs_guardado=c[13]; nxk_usuario_abortar(0);
    }
    if (servicio==1) {
        if (n>256 || !usuario_rango(p,n,0)) return;
        if (n) { memcpy(datos,(void *)(uintptr_t)p,n); nxk_pe_salida(datos,n); }
        c[11]=n; return;
    }
    if (servicio<2 || servicio>8) { c[11]=0xfffffffeu; return; }
    char nombre[24]; unsigned i;
    for (i=0;i<24;++i) {
        if (!usuario_rango(p+i,1,0) || p+i<p) return;
        nombre[i]=*(const char *)(uintptr_t)(p+i);
        if (!nombre[i]) break;
    }
    if (i==24) return;
    uint32_t buffer=c[10], cantidad=c[9];
    if (servicio==7 || servicio==8) {
        char nuevo[24]; unsigned j;
        for (j=0;j<24;++j) {
            if (buffer+j<buffer || !usuario_rango(buffer+j,1,0)) return;
            nuevo[j]=*(const char *)(uintptr_t)(buffer+j);
            if (!nuevo[j]) break;
        }
        if (j==24) return;
        if (!usuario_archivo_autorizado(c,nombre) || !usuario_archivo_autorizado(c,nuevo)) return;
        /* Ambos nombres ya son copias kernel: no se usan cadenas CPL3 dentro del FS. */
        int r=servicio==8 ? nxk_archivo_copiar(nombre,nuevo) : nxk_archivo_renombrar(nombre,nuevo);
        if (!r) c[11]=0;
        return;
    }
    if (!usuario_archivo_autorizado(c,nombre)) return;
    if (servicio==4) { if (!nxk_archivo_borrar(nombre)) c[11]=0; return; }
    if (cantidad>256 || !usuario_rango(buffer,cantidad,servicio==3 || servicio==5)) return;
    if (servicio==2 || servicio==6) {
        if (cantidad) memcpy(datos,(void *)(uintptr_t)buffer,cantidad);
        int r=servicio==6 ? nxk_archivo_agregar(nombre,datos,cantidad) : nxk_archivo_escribir(nombre,datos,cantidad);
        if (!r) c[11]=0;
    } else {
        int leidos=servicio==5 ? nxk_archivo_leer_parte(nombre,datos,cantidad,c[5]) : nxk_archivo_leer_visible(nombre,datos,cantidad);
        if (leidos<0) return;
        if (leidos) memcpy((void *)(uintptr_t)buffer,datos,(unsigned)leidos);
        c[11]=(uint32_t)leidos;
    }
}
static uint32_t fallo_vector, fallo_error, fallo_direccion, fallo_eip, fallo_ocurrido, fallo_pila;
static struct nxk_usuario_resultado historial[8];
static unsigned siguiente_resultado, cantidad_resultados;
static uint32_t secuencia_resultado;
unsigned nxk_usuario_historial_cantidad(void) { return cantidad_resultados; }
int nxk_usuario_historial(unsigned reciente,struct nxk_usuario_resultado *salida) {
    if (!salida || reciente>=cantidad_resultados) return -1;
    *salida=historial[(siguiente_resultado+7-reciente)%8]; return 0;
}
extern _Noreturn void nxk_usuario_abortar(uint32_t resultado);
static void usuario_temporizador(const uint32_t *contexto) {
    /* Segmentos 0..3, PUSHAD 4..11, EIP/CS/EFLAGS 12..14, ESP/SS 15..16. */
    if (!usuario_activo || (contexto[13]&3)!=3 || (uint32_t)(ticks-usuario_inicio)<10) return;
    if (contexto_reanudable) {
        /* IRQ0 tiene el mismo marco que INT81. Conservar EAX original: no es
         * una llamada al sistema ni una salida del proceso. EOI ya enviado. */
        memcpy(contexto_reanudable,contexto,17*sizeof(uint32_t));
        motivo_retorno=NXK_USUARIO_QUANTUM;
        fallo_eip=contexto[12]; fallo_pila=contexto[15];
        nxk_usuario_cs_guardado=contexto[13]; nxk_usuario_abortar(0);
    }
    fallo_ocurrido=1; fallo_vector=32; fallo_error=fallo_direccion=0;
    fallo_eip=contexto[12]; fallo_pila=contexto[15];
    nxk_usuario_cs_guardado=contexto[13];
    nxk_usuario_abortar(0x80000020u);
}
void nxk_usuario_atender_fallo(const uint32_t *contexto) {
    unsigned v=contexto[8];
    /* Nunca interceptar fallos de ring0, NMI ni doble fallo. */
    if (!usuario_activo || (contexto[11]&3)!=3 ||
        (v>=32 || v==2 || v==8 || v==18)) return;
    fallo_ocurrido=1; fallo_eip=contexto[10]; fallo_pila=contexto[13];
    fallo_vector=v; fallo_error=contexto[9]; fallo_direccion=0;
    if (v==14) __asm__ volatile("mov %%cr2,%0":"=r"(fallo_direccion));
    nxk_usuario_cs_guardado=contexto[11];
    nxk_usuario_abortar(0x80000000u|v);
}
/* El estado de entrada a CPL3 sigue siendo unico: turnos secuenciales. */
static int contexto_privado(void) {
    uint32_t flags,cr3,cr0;
    __asm__ volatile("pushfl; popl %0":"=r"(flags));
    __asm__ volatile("mov %%cr3,%0":"=r"(cr3));
    __asm__ volatile("mov %%cr0,%0":"=r"(cr0));
    return (cr0&0x80000000u) && (flags&0x200u) && cr3==0x200000u && !nxk_tarea_actual() && reloj_listo && emergencia_lista;
}
static int ejecutar_privado(uint32_t id,uint32_t *marco,struct nxk_usuario_resultado *salida,uint32_t *cedido,uint32_t *despertar) {
    struct nxk_espacio_info info;
    if (!salida || usuario_activo || nxk_espacio_consultar(id,&info)) return -1;
    if (!contexto_privado()) return -2;
    if ((uintptr_t)servicio_nxa_fin-(uintptr_t)servicio_nxa_inicio!=42 || servicio_nxa_inicio[25]!=0xb8) return -4;
    static unsigned char *pila;
    if (!pila) { pila=nxk_alloc(4096); if (!pila) return -3; }
    uint32_t esp0=tss_normal[1],ss0=tss_normal[2];
    struct puerta antes_salida=tabla[128],antes_servicio=tabla[129];
    uintptr_t base_anterior=usuario_base;
    memcpy(servicio_entrada,servicio_nxa_inicio,42);
    uintptr_t manejador=(uintptr_t)usuario_servicio, servicio=(uintptr_t)servicio_entrada;
    for (unsigned i=0;i<4;++i) servicio_entrada[26+i]=(unsigned char)(manejador>>(8*i));
    uintptr_t puerta_salida=(uintptr_t)nxk_usuario_salir;
    tabla[128]=(struct puerta){(uint16_t)puerta_salida,8,0,0xee,(uint16_t)(puerta_salida>>16)};
    tabla[129]=(struct puerta){(uint16_t)servicio,8,0,0xee,(uint16_t)(servicio>>16)};
    tss_normal[1]=(uint32_t)(uintptr_t)(pila+4096); tss_normal[2]=16;
    int r=nxk_espacio_activar(id);
    if (!r) {
        contexto_reanudable=marco; motivo_retorno=plazo_retorno=0;
        usuario_id_activo=marco ? id : 0;
        usuario_base=NXK_ESPACIO_CODIGO; usuario_inicio=ticks; nxk_usuario_cs_guardado=0;
        fallo_vector=fallo_error=fallo_direccion=fallo_eip=fallo_ocurrido=fallo_pila=0;
        usuario_activo=1;
        uint32_t cr0_antes;
        __asm__ volatile("mov %%cr0,%0":"=r"(cr0_antes));
        /* FPU/MMX/SSE no tienen contexto por proceso todavia: #NM en vez de
         * compartir registros. TS+MP tambien bloquean FWAIT. */
        if (marco) __asm__ volatile("mov %0,%%cr0"::"r"(cr0_antes|0xau):"memory");
        uint32_t resultado=marco ? nxk_usuario_reanudar(marco) : nxk_usuario_entrar(NXK_ESPACIO_CODIGO,NXK_ESPACIO_PILA+4096);
        if (marco) __asm__ volatile("mov %0,%%cr0"::"r"(cr0_antes):"memory");
        uint32_t regreso; __asm__ volatile("mov %%cr3,%0":"=r"(regreso));
        usuario_activo=0; usuario_id_activo=0;
        /* Restaurar el mapa antes de publicar resultado o liberar recursos. */
        int restaurado=nxk_espacio_restaurar();
        if (restaurado) __asm__ volatile("mov %0,%%cr3"::"r"(0x200000u):"memory");
        if (restaurado || regreso!=info.directorio || (nxk_usuario_cs_guardado&3)!=3) r=-5;
        if (!r) {
            struct nxk_usuario_resultado nuevo={++secuencia_resultado,resultado,fallo_ocurrido,
                fallo_vector,fallo_error,fallo_eip,fallo_direccion,fallo_pila};
            historial[siguiente_resultado]=nuevo;
            siguiente_resultado=(siguiente_resultado+1)%8;
            if (cantidad_resultados<8) ++cantidad_resultados;
        }
    }
    /* Cerrar tambien si activar fallo, sin exponer un resultado parcial. */
    tabla[128]=antes_salida; tabla[129]=antes_servicio;
    tss_normal[1]=esp0; tss_normal[2]=ss0; usuario_base=base_anterior;
    contexto_reanudable=0; usuario_id_activo=0;
    if (r) return r==-5 ? -5 : -2;
    *salida=historial[(siguiente_resultado+7)%8];
    if (cedido) *cedido=motivo_retorno;
    if (despertar) *despertar=motivo_retorno>=NXK_USUARIO_DORMIR && motivo_retorno<=NXK_USUARIO_FIN ? plazo_retorno : 0;
    return 0;
}
/* Solo datos controlados vuelven a IRET: selectores y privilegios fijos. */
static int normalizar_marco(uint32_t *m) {
    if (m[12]<NXK_ESPACIO_CODIGO || m[12]>=NXK_ESPACIO_PILA ||
        m[15]<NXK_ESPACIO_PILA || m[15]>NXK_ESPACIO_PILA+4096) return -6;
    for (unsigned i=0;i<4;++i) {
        m[i]&=0xffffu;
        if (m[i]>3 && m[i]!=43 && m[i]!=51) return -6;
    }
    m[7]=0; m[13]=43; m[16]=51;
    m[14]=(m[14]&0xcd5u)|0x202u; /* Solo aritmetica/DF; sin TF/IOPL/NT/VM. */
    return 0;
}
int nxk_usuario_ejecutar_espacio(uint32_t id,struct nxk_usuario_resultado *salida) {
    if (nxk_proceso_gestionado(id)) return -2;
    return ejecutar_privado(id,0,salida,0,0);
}
int nxk_usuario_reanudar_espacio(uint32_t id,uint32_t *marco,struct nxk_usuario_resultado *salida,uint32_t *cedido,uint32_t *despertar) {
    if (!marco || !salida || !cedido || !despertar) return -1;
    uint32_t temporal[17]; memcpy(temporal,marco,sizeof(temporal));
    int r=normalizar_marco(temporal); if (r) return r;
    struct nxk_usuario_resultado resultado; uint32_t cesion=0,plazo=0;
    r=ejecutar_privado(id,temporal,&resultado,&cesion,&plazo); if (r) return r;
    if (cesion && normalizar_marco(temporal)) return -6;
    if (cesion) memcpy(marco,temporal,sizeof(temporal));
    *salida=resultado; *cedido=cesion; *despertar=plazo; return 0;
}
static int usuario_ejecutar(unsigned modo) {
    if (usuario_activo || modo>41) return -1;
    uint32_t cr3_original; __asm__ volatile("mov %%cr3,%0":"=r"(cr3_original));
    if (cr3_original!=0x200000u) return -4;
    static uintptr_t codigo, pila_kernel, directorio_usuario, tabla_usuario, tabla_usuario_alta;
    if (!codigo) {
        unsigned char *r=nxk_alloc(8*4096);
        if (!r) return -1;
        codigo=((uintptr_t)r+4095)&~(uintptr_t)4095;
        pila_kernel=(uintptr_t)r+4*4096;
        directorio_usuario=codigo+4*4096;
        tabla_usuario=codigo+5*4096;
        tabla_usuario_alta=codigo+6*4096;
    }
    volatile uint32_t *p=(void *)0x201000,*d=(void *)0x200000;
    uint32_t *privado=(void *)directorio_usuario, *entradas_usuario=(void *)tabla_usuario;
    uint32_t *entradas_altas=(void *)tabla_usuario_alta;
    /* Solo clonamos la tabla inferior: el resto se comparte como supervisor. */
    memcpy(privado,(const void *)0x200000,4096);
    memcpy(entradas_usuario,(const void *)0x201000,4096);
    memcpy(entradas_altas,(const void *)0x202000,4096);
    /* Politica por defecto: ninguna pagina heredada obtiene acceso usuario. */
    for (unsigned i=0;i<1024;++i) { privado[i]&=~4u; entradas_usuario[i]&=~4u; entradas_altas[i]&=~4u; }
    privado[0]=(uint32_t)tabla_usuario|7;
    privado[1]=(uint32_t)tabla_usuario_alta|3;
    /* El codigo se prepara bajo el mapa kernel, que siempre es supervisor. */
    const unsigned char programa[]={0x68,42,0,0,0,0x58,0xcd,0x80,0x0f,0x0b};
    memcpy((void *)codigo,programa,sizeof(programa));
    unsigned char *bytes=(void *)codigo;
    if (modo==1) bytes[0]=0xfa; /* CLI. */
    if (modo==2) {
        const unsigned char escribir[]={0xc7,0x05,0,0,1,0,1,0,0,0};
        memcpy(bytes,escribir,sizeof(escribir));
    }
    if (modo==3) { bytes[0]=0x0f; bytes[1]=0x0b; } /* UD2. */
    if (modo==4) { const unsigned char leer[]={0xa1,0,0,0,0}; memcpy(bytes,leer,sizeof(leer)); }

    if (modo==5) {
        const unsigned char escribir_tabla[]={0xc7,0x05,0,0,0,0,1,0,0,0};
        memcpy(bytes,escribir_tabla,sizeof(escribir_tabla));
        for (unsigned i=0;i<4;++i) bytes[2+i]=(unsigned char)(directorio_usuario>>(i*8));
    }
    if (modo==6) {
        /* ESP=4 y PUSH escriben en pagina cero; la TSS mantiene pila kernel. */
        const unsigned char pila_invalida[]={0xbc,4,0,0,0,0x50};
        memcpy(bytes,pila_invalida,sizeof(pila_invalida));
    }
    if (modo==7) {
        const unsigned char escribir_codigo[]={0xc7,0x05,0,0,0,0,1,0,0,0};
        memcpy(bytes,escribir_codigo,sizeof(escribir_codigo));
        for (unsigned i=0;i<4;++i) bytes[2+i]=(unsigned char)(codigo>>(i*8));
    }
    if (modo==8) {
        /* Lectura del heap kernel: no pertenece a las dos paginas usuario. */
        const unsigned char leer_heap[]={0xa1,0,0,0x10,0};
        memcpy(bytes,leer_heap,sizeof(leer_heap));
    }
    if (modo==9) { bytes[0]=0xeb; bytes[1]=0xfe; } /* Bucle que nunca sale. */
    if (modo>=10) {
        const char mensaje[]="Hola desde usuario\n";
        memcpy(bytes+128,mensaje,sizeof(mensaje));
        const unsigned char llamada[]={0xb8,1,0,0,0,0xbb,0,0,0,0,0xb9,0,0,0,0,
            0xcd,0x81,0x3d,0,0,0,0,0x74,2,0x0f,0x0b,0xb8,42,0,0,0,0xcd,0x80};
        memcpy(bytes,llamada,sizeof(llamada));
        uint32_t puntero=(uint32_t)codigo+128, n=sizeof(mensaje)-1, esperado=n;
        if (modo==11) { puntero=0x10000; n=4; esperado=0xffffffffu; }
        if (modo==12) { n=257; esperado=0xffffffffu; }
        if (modo==13) { bytes[1]=99; esperado=0xfffffffeu; }
        if (modo==14) { puntero=0xfffffff0u; n=32; esperado=0xffffffffu; }
        if (modo==15) { puntero=(uint32_t)codigo+8191; n=2; esperado=0xffffffffu; }
        if (modo==16) { puntero=0xffffffffu; n=0; esperado=0; }
        for (unsigned i=0;i<4;++i) {
            bytes[6+i]=(unsigned char)(puntero>>(i*8));
            bytes[11+i]=(unsigned char)(n>>(i*8));
            bytes[18+i]=(unsigned char)(esperado>>(i*8));
        }
    }
    if (modo>=18) {
        const char nombre[]="usr_test"; memcpy(bytes+128,nombre,sizeof(nombre));
        const unsigned char llamada[]={0xb8,3,0,0,0,0xbb,0,0,0,0,0xb9,0,0,0,0,
            0xba,2,0,0,0,0xcd,0x81,0x3d,0,0,0,0,0x74,2,0x0f,0x0b,0xb8,42,0,0,0,0xcd,0x80};
        memcpy(bytes,llamada,sizeof(llamada));
        uint32_t destino=(uint32_t)codigo, esperado=0xffffffffu;
        if (modo==19) { destino=(uint32_t)codigo+4096; esperado=2; }
        if (modo==20) destino=0x10000;
        if (modo==21) for (unsigned i=0;i<24;++i) bytes[128+i]='a';
        if (modo==22) destino=(uint32_t)codigo+8191;
        for (unsigned i=0;i<4;++i) {
            bytes[6+i]=(unsigned char)((codigo+128)>>(8*i));
            bytes[11+i]=(unsigned char)(destino>>(8*i));
            bytes[23+i]=(unsigned char)(esperado>>(8*i));
        }
    }
    if (modo>=23) {
        const unsigned char parcial[]={0xb8,5,0,0,0,0xbb,0,0,0,0,0xb9,0,0,0,0,
            0xba,2,0,0,0,0xbe,0,0,0,0,0xcd,0x81,0x3d,0,0,0,0,0x74,2,0x0f,0x0b,
            0xb8,42,0,0,0,0xcd,0x80};
        memcpy(bytes,parcial,sizeof(parcial));
        uint32_t desplazamiento=0, esperado=2, destino=(uint32_t)codigo+4096;
        if (modo==24) { desplazamiento=1; esperado=1; }
        if (modo==25) { desplazamiento=2; esperado=0; }
        if (modo==26) { desplazamiento=3; esperado=0xffffffffu; }
        if (modo==27) { desplazamiento=0xffffffffu; esperado=0xffffffffu; }
        if (modo==28) { destino=(uint32_t)codigo; esperado=0xffffffffu; }
        for (unsigned i=0;i<4;++i) {
            bytes[6+i]=(unsigned char)((codigo+128)>>(8*i));
            bytes[11+i]=(unsigned char)(destino>>(8*i));
            bytes[21+i]=(unsigned char)(desplazamiento>>(8*i));
            bytes[28+i]=(unsigned char)(esperado>>(8*i));
        }
    }
    if (modo>=30) {
        const unsigned char llamada[]={0xb8,6,0,0,0,0xbb,0,0,0,0,0xb9,0,0,0,0,
            0xba,2,0,0,0,0xcd,0x81,0x3d,0,0,0,0,0x74,2,0x0f,0x0b,0xb8,42,0,0,0,0xcd,0x80};
        memcpy(bytes,llamada,sizeof(llamada)); memcpy(bytes+200,"CD",2);
        uint32_t puntero=(uint32_t)codigo+200, cantidad=2, esperado=0;
        if (modo==31) { puntero=0x10000; esperado=0xffffffffu; }
        if (modo==32) { cantidad=257; esperado=0xffffffffu; }
        if (modo==33) { cantidad=256; esperado=0xffffffffu; }
        if (modo>=34) {
            bytes[1]=7; cantidad=0;
            memcpy(bytes+200,modo==34 ? "usr_taken" : "usr_final",10);
            if (modo==34) esperado=0xffffffffu;
            if (modo==36) { memcpy(bytes+128,"usr_final",10); puntero=0x10000; esperado=0xffffffffu; }
        }
        if (modo>=37) {
            bytes[1]=8; cantidad=0;
            memcpy(bytes+128,"usr_final",10); memcpy(bytes+200,"usr_copy",9);
            if (modo==38) esperado=0xffffffffu; /* Destino creado por modo 37. */
            if (modo==39) { puntero=0x10000; esperado=0xffffffffu; }
            if (modo==40) { memset(bytes+200,'x',24); esperado=0xffffffffu; }
            if (modo==41) esperado=0xffffffffu;
        }
        for (unsigned i=0;i<4;++i) {
            bytes[6+i]=(unsigned char)((modo==41 ? 0x10000 : codigo+128)>>(i*8));
            bytes[11+i]=(unsigned char)(puntero>>(i*8));
            bytes[16+i]=(unsigned char)(cantidad>>(i*8));
            bytes[23+i]=(unsigned char)(esperado>>(i*8));
        }
    }
    uintptr_t virtual_codigo=codigo;
    uint32_t *entradas_activas=entradas_usuario;
    if (modo==17 || modo==29) {
        extern const unsigned char usuario_nxf_inicio[], usuario_nxf_fin[];
        const unsigned char *fuente=modo==29 ? usuario_carga : usuario_nxf_inicio;
        size_t n=modo==29 ? usuario_carga_longitud : (uintptr_t)usuario_nxf_fin-(uintptr_t)usuario_nxf_inicio;
        if (!n || n>4096) return -9;
        memset(bytes,0,4096); memcpy(bytes,fuente,n);
        virtual_codigo=0x400000; entradas_activas=entradas_altas;
        privado[1]|=4;
    }
    unsigned indice=(virtual_codigo/4096)%1024;
    memset((void *)(codigo+4096),0,4096);
    uint32_t antes_codigo=p[codigo/4096], antes_pila=p[codigo/4096+1];
    uint32_t antes_directorio=d[codigo>>22], esp0=tss_normal[1], ss0=tss_normal[2];
    struct puerta antes_puerta=tabla[128], antes_servicio=tabla[129];
    if ((uintptr_t)servicio_nxa_fin-(uintptr_t)servicio_nxa_inicio!=42 || servicio_nxa_inicio[25]!=0xb8) return -8;
    memcpy(servicio_entrada,servicio_nxa_inicio,42);
    uintptr_t manejador=(uintptr_t)usuario_servicio, puerta_servicio=(uintptr_t)servicio_entrada;
    for (unsigned i=0;i<4;++i) servicio_entrada[26+i]=(unsigned char)(manejador>>(i*8));
    tabla[129]=(struct puerta){(uint16_t)puerta_servicio,8,0,0xee,(uint16_t)(puerta_servicio>>16)};
    uintptr_t salida=(uintptr_t)nxk_usuario_salir;
    tabla[128]=(struct puerta){(uint16_t)salida,8,0,0xee,(uint16_t)(salida>>16)};
    tss_normal[1]=(uint32_t)pila_kernel; tss_normal[2]=16;
    entradas_activas[indice]=(uint32_t)codigo|5;
    entradas_activas[indice+1]=(uint32_t)(codigo+4096)|7;
    __asm__ volatile("mov %0,%%cr3" :: "r"((uint32_t)directorio_usuario) : "memory");
    nxk_usuario_cs_guardado=0;
    usuario_base=virtual_codigo; usuario_inicio=ticks;
    usuario_activo=1; fallo_vector=fallo_error=fallo_direccion=fallo_eip=fallo_ocurrido=fallo_pila=0;
    uint32_t resultado=nxk_usuario_entrar((uint32_t)virtual_codigo,(uint32_t)(virtual_codigo+8192));
    uint32_t cr3_regreso; __asm__ volatile("mov %%cr3,%0":"=r"(cr3_regreso));
    __asm__ volatile("mov %0,%%cr3" :: "r"(cr3_original) : "memory");
    usuario_activo=0;
    /* Retirar la puerta temporal y revocar permisos en la copia inactiva. */
    entradas_activas[indice]&=~4u; entradas_activas[indice+1]&=~4u;
    privado[0]&=~4u; privado[1]&=~4u;
    tabla[128]=antes_puerta; tabla[129]=antes_servicio;
    tss_normal[1]=esp0; tss_normal[2]=ss0;
    __asm__ volatile("mov %0,%%cr3" :: "r"(0x200000u) : "memory");
    /* A/D pueden cambiar por acceso CPU; permisos y marcos no deben cambiar. */
    if (cr3_regreso!=directorio_usuario ||
        ((p[codigo/4096]^antes_codigo)&~0x60u) ||
        ((p[codigo/4096+1]^antes_pila)&~0x60u) ||
        ((d[codigo>>22]^antes_directorio)&~0x60u)) return -5;
    historial[siguiente_resultado]=(struct nxk_usuario_resultado){
        ++secuencia_resultado,resultado,fallo_ocurrido,fallo_vector,fallo_error,fallo_eip,fallo_direccion,fallo_pila};
    siguiente_resultado=(siguiente_resultado+1)%8;
    if (cantidad_resultados<8) ++cantidad_resultados;
    if (modo==7 && (fallo_vector!=14 || fallo_error!=7 || fallo_direccion!=codigo)) return -7;
    if (modo==5 && (fallo_vector!=14 || fallo_error!=7 || fallo_direccion!=directorio_usuario)) return -6;
    if ((nxk_usuario_cs_guardado&3)!=3) return -2;
    if (modo && modo<10) return resultado==(0x80000000u|fallo_vector) ? (int)fallo_vector : -3;
    return resultado==((modo==17 || modo==29) ? 0u : 42u) ? 0 : -2;
}
static uint32_t usuario_u32(const unsigned char *p) {
    return p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}
static int usuario_validar_nxe(const unsigned char *p,size_t n) {
    if (!p || n<64 || n>4096 || usuario_activo) return -1;
    if (usuario_u32(p)!=0x0045584e || usuario_u32(p+4)!=0x00010002u ||
        usuario_u32(p+8)!=64 || usuario_u32(p+12)!=0x400000 || usuario_u32(p+16) ||
        !usuario_u32(p+20) || usuario_u32(p+20)!=n-64 || usuario_u32(p+24)!=8192 ||
        usuario_u32(p+32)!=1) return -2;
    for (unsigned i=36;i<64;++i) if (p[i]) return -2;
    uint32_t h=2166136261u; for (size_t i=64;i<n;++i) h=(h^p[i])*16777619u;
    if (h!=usuario_u32(p+28)) return -3;
    return 0;
}
int nxk_usuario_cargar_nxe(const unsigned char *p,size_t n) {
    int r=usuario_validar_nxe(p,n); if (r) return r;
    usuario_carga=p+64; usuario_carga_longitud=n-64;
    int resultado=usuario_ejecutar(29);
    usuario_carga=0; usuario_carga_longitud=0; return resultado;
}
int nxk_usuario_crear_espacio_nxe(const unsigned char *p,size_t n,uint32_t *id) {
    if (!id) return -1;
    int r=usuario_validar_nxe(p,n); if (r) return r;
    if (!contexto_privado()) return -2;
    return nxk_espacio_crear(p+64,n-64,id);
}
static unsigned char *imagen_archivo;
int nxk_usuario_preparar_archivo(const char *nombre,uint32_t *id) {
    if (!nombre || !id || usuario_activo) return -1;
    if (!contexto_privado()) return -2;
    if (!imagen_archivo) imagen_archivo=nxk_alloc(4096);
    if (!imagen_archivo) return -3;
    int n=nxk_volumen_leer(nombre,imagen_archivo,4096);
    return n<0 ? -4 : nxk_usuario_crear_espacio_nxe(imagen_archivo,(size_t)n,id);
}
int nxk_usuario_ejecutar_archivo(const char *nombre) {
    if (!nombre || usuario_activo) return -1;
    if (!imagen_archivo) imagen_archivo=nxk_alloc(4096);
    if (!imagen_archivo) return -1;
    int n=nxk_volumen_leer(nombre,imagen_archivo,4096);
    return n<0 ? -4 : nxk_usuario_cargar_nxe(imagen_archivo,(size_t)n);
}
int nxk_usuario_nextry(void) { return usuario_ejecutar(17); }
int nxk_usuario_demo(void) { return usuario_ejecutar(0); }
int nxk_usuario_probar_servicios(void) {
    if (usuario_ejecutar(10)) return 1;
    size_t antes=nxk_heap_used();
    for (unsigned modo=11;modo<=16;++modo)
        if (usuario_ejecutar(modo) || nxk_heap_used()!=antes) return 2;
    char datos[4];
    if (nxk_archivo_leer("usr_test",datos,4)!=-2 || nxk_archivo_leer("usr_final",datos,4)!=-2 ||
        nxk_archivo_leer("usr_taken",datos,4)!=-2 || nxk_archivo_leer("usr_copy",datos,4)!=-2 || nxk_archivo_escribir("usr_test","AB",2)) return 3;
    if (nxk_archivo_escribir("usr_taken",0,0)) { nxk_archivo_borrar("usr_test"); return 3; }
    antes=nxk_heap_used(); int fallo=0;
    for (unsigned modo=18;modo<=28;++modo)
        if (usuario_ejecutar(modo) || nxk_heap_used()!=antes) { fallo=4; break; }
    if (!fallo) for (unsigned modo=30;modo<=41;++modo) {
        if (usuario_ejecutar(modo) || nxk_heap_used()!=antes ||
            nxk_archivo_leer(modo>=35 ? "usr_final" : "usr_test",datos,4)!=4 ||
            datos[0]!='A' || datos[1]!='B' || datos[2]!='C' || datos[3]!='D') { fallo=6; break; }
        if (modo>=37 && (nxk_archivo_leer("usr_copy",datos,4)!=4 ||
            datos[0]!='A' || datos[1]!='B' || datos[2]!='C' || datos[3]!='D')) { fallo=8; break; }
        if (nxk_archivo_leer("usr_taken",datos,4)!=0) { fallo=7; break; }
    }
    nxk_archivo_borrar("usr_test"); nxk_archivo_borrar("usr_final"); nxk_archivo_borrar("usr_taken"); nxk_archivo_borrar("usr_copy");
    return fallo;
}
int nxk_usuario_caso_bucle(void) { return usuario_ejecutar(9)==32 ? 0 : -1; }
int nxk_usuario_caso_nulo(void) {
    return usuario_ejecutar(4)==14 && fallo_error==4 && !fallo_direccion ? 0 : -1;
}
int nxk_usuario_comprobar_fallos(void) {
    if (nxk_usuario_demo()) return 1;
    size_t antes=nxk_heap_used();
    const unsigned vectores[]={13,14,6,14,14,14,14,14,32}, errores[]={0,7,0,4,7,6,7,5,0};
    for (unsigned modo=1;modo<=9;++modo) {
        if (usuario_ejecutar(modo)!=(int)vectores[modo-1] || fallo_error!=errores[modo-1]) return 2;
        struct nxk_usuario_resultado registro;
        if (nxk_usuario_historial(0,&registro) || !registro.fallo ||
            registro.vector!=vectores[modo-1] || !registro.eip) return 6;
        if (modo==2 && fallo_direccion!=0x10000) return 3;
        if (modo==4 && fallo_direccion!=0) return 4;
        if (modo==6 && (fallo_direccion || fallo_pila!=4)) return 10;
        if (modo==8 && fallo_direccion!=0x100000) return 11;
        if (nxk_usuario_demo() || nxk_heap_used()!=antes) return 5;
        if (nxk_usuario_historial(0,&registro) || registro.fallo || registro.vector ||
            registro.error || registro.eip || registro.direccion || registro.pila || registro.salida!=42) return 7;
    }
    struct nxk_usuario_resultado registro;
    if (cantidad_resultados!=8 || nxk_usuario_historial(8,&registro)!=-1 ||
        nxk_usuario_historial(0,0)!=-1) return 8;
    for (unsigned i=0;i<8;++i) {
        if (nxk_usuario_historial(i,&registro) ||
            registro.secuencia!=(uint32_t)(secuencia_resultado-i)) return 9;
    }
    return 0;
}
void nxk_excepciones_iniciar(void) {
    uintptr_t longitud=(uintptr_t)excepcion_nxa_fin-(uintptr_t)excepcion_nxa_inicio;
    /* Plantilla NXA: vector en byte 3, segmentos normalizados y B8 en 28. */
    if (longitud!=41 || excepcion_nxa_inicio[0]!=0x6a ||
        excepcion_nxa_inicio[2]!=0x6a || excepcion_nxa_inicio[28]!=0xb8)
        nxk_es_detener();
    uintptr_t manejador=(uintptr_t)nxk_excepcion_fatal;
    for (unsigned v=0;v<33;++v) {
        for (uintptr_t i=0;i<longitud;++i) entrada[v][i]=excepcion_nxa_inicio[i];
        entrada[v][3]=(unsigned char)v;
        for (unsigned i=0;i<4;++i) entrada[v][29+i]=(unsigned char)(manejador>>(8*i));
    }
    uint16_t selector;
    __asm__ volatile("mov %%cs, %0" : "=r"(selector));
    for (unsigned i=0;i<256;++i) {
        unsigned v=i<32 ? i : 32;
        /* Los vectores con error CPU omiten el cero auxiliar de la plantilla. */
        unsigned error=i==8 || (i>=10 && i<=14) || i==17 || i==21 || i==29 || i==30;
        uintptr_t direccion=(uintptr_t)entrada[v]+(error ? 2 : 0);
        tabla[i]=(struct puerta){(uint16_t)direccion,selector,0,0x8e,(uint16_t)(direccion>>16)};
    }
    struct descriptor d={sizeof(tabla)-1,(uint32_t)(uintptr_t)tabla};
    __asm__ volatile("lidt %0" :: "m"(d) : "memory");
}
#else
unsigned nxk_usuario_historial_cantidad(void) { return 0; }
int nxk_usuario_historial(unsigned i,struct nxk_usuario_resultado *s) { (void)i;(void)s;return -64; }
int nxk_usuario_probar_servicios(void) { return -64; }
int nxk_usuario_caso_bucle(void) { return -64; }
int nxk_usuario_caso_nulo(void) { return -64; }
void nxk_usuario_atender_fallo(const uint32_t *p) { (void)p; }
int nxk_usuario_comprobar_fallos(void) { return -64; }
int nxk_usuario_cargar_nxe(const unsigned char *p,size_t n) { (void)p;(void)n;return -64; }
int nxk_usuario_ejecutar_archivo(const char *n) { (void)n;return -64; }
int nxk_usuario_ejecutar_espacio(uint32_t id,struct nxk_usuario_resultado *s) { (void)id;(void)s;return -64; }
int nxk_usuario_reanudar_espacio(uint32_t id,uint32_t *m,struct nxk_usuario_resultado *s,uint32_t *c,uint32_t *d) { (void)id;(void)m;(void)s;(void)c;(void)d;return -64; }
int nxk_usuario_crear_espacio_nxe(const unsigned char *p,size_t n,uint32_t *id) { (void)p;(void)n;(void)id;return -64; }
int nxk_usuario_preparar_archivo(const char *n,uint32_t *id) { (void)n;(void)id;return -64; }
int nxk_usuario_nextry(void) { return -64; }
int nxk_usuario_demo(void) { return -64; }
void nxk_excepciones_iniciar(void) {}
uint32_t nxk_reloj_ticks(void) { return 0; }
void nxk_reloj_iniciar(void) {}
#endif
