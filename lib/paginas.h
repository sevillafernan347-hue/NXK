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
void *nxk_pagina_reservar(void); /* Devuelve cero si no quedan paginas; no las borra. */
int nxk_pagina_liberar(void *pagina); /* Rechaza paginas ajenas, desalineadas y doble liberacion. */
uint32_t nxk_paginas_libres(void);
uint32_t nxk_paginas_total(void);
#endif
