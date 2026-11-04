/* Semaforos contadores kernel: una CPU, fuera de IRQ, IDs propios.
 * No son mutex con propietario: cancelar no devuelve permisos adquiridos. */
#ifndef NXK_SEMAFOROS_H
#define NXK_SEMAFOROS_H
#include "nxk.h"
#define NXK_SEMAFOROS_MAX 8u
struct nxk_semaforo_info { uint32_t id, disponibles, maximo, esperadores; };
int nxk_semaforo_crear(uint32_t inicial,uint32_t maximo,uint32_t *id);
int nxk_semaforo_consultar(uint32_t id,struct nxk_semaforo_info *salida);
/* Adquirir solo desde tarea con IF: 0 exito, 1 plazo/consulta vacia, -1 invalido. */
int nxk_semaforo_adquirir(uint32_t id,uint32_t ticks);
/* Publicar: -1 ID invalido, -2 si ya esta al maximo; no desborda. */
int nxk_semaforo_publicar(uint32_t id);
/* Destruir: -3 con esperadores, incluso despertados que aun no reanudaron. */
int nxk_semaforo_destruir(uint32_t id);
int nxk_semaforos_demo(void);
int nxk_semaforos_probar(void);
#endif
