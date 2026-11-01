/* Operaciones internas de paginacion x86; solo kernel y una CPU. */
#ifndef NXK_PAGINACION_H
#define NXK_PAGINACION_H
#include "nxk.h"
/* Marca una pagina del heap como ausente. No libera su reserva fisica. */
int nxk_paginacion_guarda(void *pagina);
int nxk_paginacion_es_guarda(void *pagina);
#endif
