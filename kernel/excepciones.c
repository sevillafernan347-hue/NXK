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
#endif
