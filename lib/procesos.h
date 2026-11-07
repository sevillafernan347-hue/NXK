/* Procesos CPL3 con cesion y preempcion: cuatro contextos CPU privados.
 * Puente C, una CPU, fuera de IRQ/tareas. El servicio INT81/9 cede el turno. */
#ifndef NXK_PROCESOS_H
#define NXK_PROCESOS_H
#include "usuario.h"
#include "espacios.h"
enum nxk_proceso_estado { NXK_PROCESO_LIBRE, NXK_PROCESO_LISTO,
    NXK_PROCESO_EJECUTANDO, NXK_PROCESO_TERMINADO, NXK_PROCESO_FALLIDO, NXK_PROCESO_DORMIDO };
struct nxk_proceso_info {
    uint32_t id,estado,turnos,cesiones,preempciones,error_motor,dormidas,despertar;
    struct nxk_usuario_resultado ultimo;
};
/* Crear/cargar conserva salida al fallar. IDs son los IDs completos del espacio.
 * Rechazos: -1 contrato/ID, -2 estado/pool, -3 contexto; memoria/motor negativos. */
int nxk_proceso_crear(const void *codigo,size_t longitud,uint32_t *id);
int nxk_proceso_cargar_archivo(const char *nombre,uint32_t *id);
int nxk_proceso_consultar(uint32_t id,struct nxk_proceso_info *salida);
/* -2 si el slot esta libre. Errores nunca modifican salida. */
int nxk_proceso_enumerar(unsigned slot,struct nxk_proceso_info *salida);
int nxk_proceso_paso(uint32_t id);
/* Hasta1024 turnos: 0 sin pendientes, 1 presupuesto, 2 solo dormidos.
 * Un fallo CPU de una app no impide ejecutar otras. Error del motor negativo. */
int nxk_procesos_ejecutar(uint32_t presupuesto);
int nxk_proceso_liberar(uint32_t id);
/* Impide arrancar por la ruta legacy un espacio con contexto CPU conservado. */
int nxk_proceso_gestionado(uint32_t id);
int nxk_procesos_demo(void);
int nxk_preempcion_demo(void);
int nxk_proceso_dormir_demo(void);
#ifdef NXK_TEST
int nxk_procesos_probar(void);
int nxk_preempcion_probar(void);
int nxk_proceso_dormir_probar(void);
#endif
#endif
