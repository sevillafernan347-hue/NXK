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
/* Lista de 1..32 direcciones fisicas, sin exigir contiguidad. Copia y valida
 * toda la lista antes de borrar paginas; el rechazo no tiene efectos.
 * La lista kernel confiable puede residir dentro de una pagina que libera.
 * 0 exito; -1 null/cantidad/direccion/duplicados, -2 alguna no prestada,
 * -3 no iniciado, -64 x64. Una CPU, sin concurrencia ni llamadas desde IRQ. */
int nxk_paginas_liberar_lista(const uint32_t *paginas,uint32_t cantidad);
uint32_t nxk_paginas_libres(void);
uint32_t nxk_paginas_total(void);
/* Instantanea en paginas de 4 KiB. total es la capacidad inicial; excluidas
 * cuenta solo las retiradas despues por reservar_region, no huecos del BIOS.
 * mayor_bloque no garantiza una alineacion superior a una pagina. */
struct nxk_diagnostico_paginas {
    uint32_t total, libres, prestadas, excluidas, mayor_bloque, bloques_libres;
};
/* Consulta explicita O(paginas), sin asignaciones ni cambios del gestor.
 * Solo kernel, una CPU, sin concurrencia con el gestor ni llamadas desde IRQ.
 * salida debe apuntar a memoria kernel escribible fuera de los metadatos.
 * 0 exito; -1 nulo, -3 no iniciado, -2 incoherencia, -64 x64 no soportado.
 * En error no modifica salida. */
int nxk_paginas_diagnostico(struct nxk_diagnostico_paginas *salida);
#endif
