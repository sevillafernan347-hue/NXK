/* Memoria virtual privada x86: base para futuros procesos, no planificador.
 * Puente C, una CPU, fuera de IRQ y de tareas; punteros kernel confiables. */
#ifndef NXK_ESPACIOS_H
#define NXK_ESPACIOS_H
#include "nxk.h"
#define NXK_ESPACIOS_MAX 4u
#define NXK_ESPACIO_CODIGO 0x400000u
#define NXK_ESPACIO_PILA 0x401000u
struct nxk_espacio_info { uint32_t id, directorio, codigo_fisico, pila_fisica; };
/* Crear: 1..4096 bytes, codigo RO, pila RW en cero; salida intacta al fallar.
 * 0 exito; -1 contrato/ID; -2 tabla llena/activo al destruir;
 * -3 contexto no permitido; -4 memoria fisica insuficiente. */
int nxk_espacio_crear(const void *codigo,size_t longitud,uint32_t *id);
int nxk_espacio_consultar(uint32_t id,struct nxk_espacio_info *salida);
/* Activa desde kernel u otro espacio propio, refresca mapas kernel y CR3.
 * NO cambia registros CPU, TSS ni pila de retorno de un proceso. */
int nxk_espacio_activar(uint32_t id);
int nxk_espacio_restaurar(void);
int nxk_espacio_destruir(uint32_t id);
int nxk_espacios_demo(void);
#ifdef NXK_TEST
int nxk_espacios_probar(void);
#endif
#endif
