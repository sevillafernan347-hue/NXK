/* Paginas fisicas de 4 KiB. Solo kernel x86, sin concurrencia ni memoria virtual. */
#ifndef NXK_PAGINAS_H
#define NXK_PAGINAS_H
#include "nxk.h"
struct __attribute__((packed)) nxk_region_ram {
    uint64_t base, longitud;
    uint32_t tipo;
};
int nxk_paginas_iniciar(const struct nxk_region_ram *mapa, unsigned cantidad);
int nxk_paginas_bios(void);
void *nxk_pagina_reservar(void); /* Devuelve cero si no quedan paginas; las entrega en cero. */
int nxk_pagina_liberar(void *pagina); /* Rechaza paginas ajenas, desalineadas y doble liberacion. */
/* Excluye del asignador el rango tocado, redondeando hacia afuera a 4 KiB. */
int nxk_paginas_reservar_region(uint32_t inicio, uint32_t longitud);
/* Bloques fisicos: cantidad y alineacion en paginas. Alineacion potencia de 2.
 * Liberar valida todo el rango antes de borrarlo; no comprueba identidad de bloque.
 * Solo una CPU, fuera de IRQ; no ofrece sincronizacion concurrente. */
void *nxk_paginas_reservar_bloque(uint32_t cantidad,uint32_t alineacion);
int nxk_paginas_liberar_bloque(void *inicio,uint32_t cantidad);
uint32_t nxk_paginas_libres(void);
uint32_t nxk_paginas_total(void);
#endif
