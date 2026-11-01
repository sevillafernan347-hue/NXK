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
extern void nxk_excepcion_fatal(void);
static unsigned char entrada[32];
void nxk_excepciones_iniciar(void) {
    uintptr_t longitud=(uintptr_t)excepcion_nxa_fin-(uintptr_t)excepcion_nxa_inicio;
    /* Contrato del stub: B8 en offset 6, inmediato de 32 bits en offset 7. */
    if (longitud<13 || longitud>sizeof(entrada) || excepcion_nxa_inicio[6]!=0xb8)
        nxk_es_detener();
    for (uintptr_t i=0;i<longitud;++i) entrada[i]=excepcion_nxa_inicio[i];
    uintptr_t manejador=(uintptr_t)nxk_excepcion_fatal;
    for (unsigned i=0;i<4;++i) entrada[7+i]=(unsigned char)(manejador>>(8*i));
    uintptr_t direccion=(uintptr_t)entrada;
    uint16_t selector;
    __asm__ volatile("mov %%cs, %0" : "=r"(selector));
    for (unsigned i=0;i<256;++i) {
        /* Por ahora todas las puertas llevan al mismo aviso fatal.
           No distinguimos el numero de excepcion ni reanudamos la ejecucion. */
        tabla[i]=(struct puerta){(uint16_t)direccion,selector,0,0x8e,(uint16_t)(direccion>>16)};
    }
    struct descriptor d={sizeof(tabla)-1,(uint32_t)(uintptr_t)tabla};
    __asm__ volatile("lidt %0" :: "m"(d) : "memory");
}
#else
void nxk_excepciones_iniciar(void) {}
#endif
