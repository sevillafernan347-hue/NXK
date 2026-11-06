/* Operaciones internas de paginacion x86; solo kernel y una CPU. */
#ifndef NXK_PAGINACION_H
#define NXK_PAGINACION_H
#include "nxk.h"
/* Inicializacion unica: -1 mapa BIOS invalido, -2 si ya esta activa. */
int nxk_paginacion_iniciar(void);
#ifdef NXK_TEST
int nxk_paginacion_probar(void);
#endif
/* Marca una pagina del heap como ausente. Solo CR3 kernel; no libera reserva. */
int nxk_paginacion_guarda(void *pagina);
int nxk_paginacion_es_guarda(void *pagina);
uint32_t nxk_paginacion_guardas(void);
#endif
