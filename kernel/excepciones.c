/* GUIA DE LECTURA NXK - kernel/excepciones.c
 * Construye la IDT: tabla que indica a la CPU donde ir ante una excepcion. Solo implementada para x86.
 * Contexto y orden de estudio: ver LEER-CODIGO.md en la carpeta principal.
 */
#include <stdint.h>
#include "nxk_es.h"
#include "nxk.h"
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
static unsigned char reloj_entrada[18];
static volatile uint32_t ticks;
static void puerto(uint16_t p,uint8_t v) {
    __asm__ volatile("outb %0,%1" :: "a"(v),"Nd"(p));
    __asm__ volatile("outb %%al,$0x80" :: "a"(0));
}
void nxk_reloj_irq(void) { ++ticks; puerto(0x20,0x20); }
uint32_t nxk_reloj_ticks(void) { return ticks; }
void nxk_reloj_iniciar(void) {
    __asm__ volatile("cli" ::: "memory");
    if ((uintptr_t)reloj_nxa_fin-(uintptr_t)reloj_nxa_inicio!=18 ||
        reloj_nxa_inicio[7]!=0xb8 || reloj_nxa_inicio[17]!=0xcf)
        nxk_es_detener();
    for (unsigned i=0;i<18;++i) reloj_entrada[i]=reloj_nxa_inicio[i];
    uintptr_t f=(uintptr_t)nxk_reloj_irq, e=(uintptr_t)reloj_entrada;
    for (unsigned i=0;i<4;++i) reloj_entrada[8+i]=(unsigned char)(f>>(8*i));
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
void nxk_excepciones_iniciar(void) {}
uint32_t nxk_reloj_ticks(void) { return 0; }
void nxk_reloj_iniciar(void) {}
#endif
