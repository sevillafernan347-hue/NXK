/* Mutex kernel no recursivos con propietario y deteccion de abandono.
 * Solo una CPU, fuera de IRQ. La recuperacion de datos compete al llamador. */
#ifndef NXK_MUTEX_H
#define NXK_MUTEX_H
#include "nxk.h"
#define NXK_MUTEX_MAX 8u
struct nxk_mutex_info { uint32_t id, propietario, abandonado, irrecuperable, esperadores; };
int nxk_mutex_crear(uint32_t *id);
int nxk_mutex_consultar(uint32_t id,struct nxk_mutex_info *salida);
/* 0 adquirido, 2 adquirido abandonado (requiere consistente), 1 plazo.
 * -1 contrato/ID, -2 recursion, -4 ciclo, -5 irrecuperable. Solo tarea IF. */
int nxk_mutex_adquirir(uint32_t id,uint32_t ticks);
/* Solo propietario: -2 si es ajeno, -3 si no requiere reparacion. */
int nxk_mutex_consistente(uint32_t id);
/* Solo propietario. Si no reparo abandono, libera y lo marca irrecuperable. */
int nxk_mutex_liberar(uint32_t id);
/* -3 si tiene propietario o referencias de espera aun sin consumir. */
int nxk_mutex_destruir(uint32_t id);
int nxk_mutex_demo(void);
int nxk_mutex_recuperacion_demo(void);
int nxk_mutex_probar(void);
#endif
