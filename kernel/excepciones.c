/* GUIA DE LECTURA NXK - kernel/excepciones.c
 * Construye la IDT: tabla que indica a la CPU donde ir ante una excepcion. Solo implementada para x86.
 * Contexto y orden de estudio: ver LEER-CODIGO.md en la carpeta principal.
 */
#include <stdint.h>
#include "nxk_es.h"
#include "nxk.h"
#include "usuario.h"
#include "archivos.h"
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
static unsigned char entrada[33][32];
/* Plantilla generada por NEXTRY ASM; el puente resuelve la llamada C. */
extern const unsigned char reloj_nxa_inicio[], reloj_nxa_fin[];
static unsigned char reloj_entrada[42];
static volatile uint32_t ticks;
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
    ticks=0;
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
static uint32_t usuario_inicio;
static uintptr_t usuario_base;
extern const unsigned char servicio_nxa_inicio[], servicio_nxa_fin[];
static unsigned char servicio_entrada[42];
extern void nxk_pe_salida(const char *datos,uint32_t n);
static int usuario_rango(uint32_t p,uint32_t n,int escritura) {
    if (!n) return 1;
    uintptr_t minimo=usuario_base+(escritura ? 4096 : 0);
    return p>=minimo && p-usuario_base<8192 && n<=8192-(p-usuario_base);
}
static void usuario_servicio(uint32_t *c) {
    uint32_t servicio=c[11], p=c[8], n=c[10];
    c[11]=0xffffffffu;
    if (!usuario_activo || (c[13]&3)!=3) return;
    char datos[256];
    if (servicio==1) {
        if (n>256 || !usuario_rango(p,n,0)) return;
        if (n) { memcpy(datos,(void *)(uintptr_t)p,n); nxk_pe_salida(datos,n); }
        c[11]=n; return;
    }
    if (servicio<2 || servicio>5) { c[11]=0xfffffffeu; return; }
    char nombre[24]; unsigned i;
    for (i=0;i<24;++i) {
        if (!usuario_rango(p+i,1,0) || p+i<p) return;
        nombre[i]=*(const char *)(uintptr_t)(p+i);
        if (!nombre[i]) break;
    }
    if (i==24) return;
    uint32_t buffer=c[10], cantidad=c[9];
    if (servicio==4) { if (!nxk_archivo_borrar(nombre)) c[11]=0; return; }
    if (cantidad>256 || !usuario_rango(buffer,cantidad,servicio==3 || servicio==5)) return;
    if (servicio==2) {
        if (cantidad) memcpy(datos,(void *)(uintptr_t)buffer,cantidad);
        if (!nxk_archivo_escribir(nombre,datos,cantidad)) c[11]=0;
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
    fallo_ocurrido=1; fallo_vector=32; fallo_error=fallo_direccion=0;
    fallo_eip=contexto[12]; fallo_pila=contexto[15];
    nxk_usuario_cs_guardado=contexto[13];
    nxk_usuario_abortar(0x80000020u);
}
void nxk_usuario_atender_fallo(const uint32_t *contexto) {
    unsigned v=contexto[8];
    /* Nunca interceptar fallos de ring0, NMI ni doble fallo. */
    if (!usuario_activo || (contexto[11]&3)!=3 ||
        (v!=0 && v!=6 && v!=13 && v!=14)) return;
    fallo_ocurrido=1; fallo_eip=contexto[10]; fallo_pila=contexto[13];
    fallo_vector=v; fallo_error=contexto[9]; fallo_direccion=0;
    if (v==14) __asm__ volatile("mov %%cr2,%0":"=r"(fallo_direccion));
    nxk_usuario_cs_guardado=contexto[11];
    nxk_usuario_abortar(0x80000000u|v);
}
static int usuario_ejecutar(unsigned modo) {
    if (usuario_activo || modo>28) return -1;
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
    uintptr_t virtual_codigo=codigo;
    uint32_t *entradas_activas=entradas_usuario;
    if (modo==17) {
        extern const unsigned char usuario_nxf_inicio[], usuario_nxf_fin[];
        size_t n=(uintptr_t)usuario_nxf_fin-(uintptr_t)usuario_nxf_inicio;
        if (!n || n>4096) return -9;
        memset(bytes,0,4096); memcpy(bytes,usuario_nxf_inicio,n);
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
    return resultado==42 ? 0 : -2;
}
int nxk_usuario_nextry(void) { return usuario_ejecutar(17); }
int nxk_usuario_demo(void) { return usuario_ejecutar(0); }
int nxk_usuario_probar_servicios(void) {
    if (usuario_ejecutar(10)) return 1;
    size_t antes=nxk_heap_used();
    for (unsigned modo=11;modo<=16;++modo)
        if (usuario_ejecutar(modo) || nxk_heap_used()!=antes) return 2;
    char datos[2];
    if (nxk_archivo_leer("usr_test",datos,2)!=-2 || nxk_archivo_escribir("usr_test","AB",2)) return 3;
    antes=nxk_heap_used(); int fallo=0;
    for (unsigned modo=18;modo<=28;++modo)
        if (usuario_ejecutar(modo) || nxk_heap_used()!=antes) { fallo=4; break; }
    if (nxk_archivo_borrar("usr_test")) fallo=5;
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
    /* Plantilla NXA: vector en byte 3, B8 en 16 y direccion C en 17. */
    if (longitud!=29 || excepcion_nxa_inicio[0]!=0x6a ||
        excepcion_nxa_inicio[2]!=0x6a || excepcion_nxa_inicio[16]!=0xb8)
        nxk_es_detener();
    uintptr_t manejador=(uintptr_t)nxk_excepcion_fatal;
    for (unsigned v=0;v<33;++v) {
        for (uintptr_t i=0;i<longitud;++i) entrada[v][i]=excepcion_nxa_inicio[i];
        entrada[v][3]=(unsigned char)v;
        for (unsigned i=0;i<4;++i) entrada[v][17+i]=(unsigned char)(manejador>>(8*i));
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
int nxk_usuario_nextry(void) { return -64; }
int nxk_usuario_demo(void) { return -64; }
void nxk_excepciones_iniciar(void) {}
uint32_t nxk_reloj_ticks(void) { return 0; }
void nxk_reloj_iniciar(void) {}
#endif
