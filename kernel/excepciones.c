/* GUIA DE LECTURA NXK - kernel/excepciones.c
 * Construye la IDT: tabla que indica a la CPU donde ir ante una excepcion. Solo implementada para x86.
 * Contexto y orden de estudio: ver LEER-CODIGO.md en la carpeta principal.
 */
#include <stdint.h>
#include "nxk_es.h"
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
