/* Procesos CPL3 con cesion y preempcion: cuatro contextos CPU privados.
 * Puente C, una CPU, fuera de IRQ/tareas. El servicio INT81/9 cede el turno. */
#ifndef NXK_PROCESOS_H
#define NXK_PROCESOS_H
#include "usuario.h"
#include "espacios.h"
enum nxk_proceso_estado { NXK_PROCESO_LIBRE, NXK_PROCESO_LISTO,
    NXK_PROCESO_EJECUTANDO, NXK_PROCESO_TERMINADO, NXK_PROCESO_FALLIDO, NXK_PROCESO_DORMIDO,
    NXK_PROCESO_ESPERANDO_MENSAJE, NXK_PROCESO_CANCELADO };
#define NXK_PROCESO_BUZON_MAX 4u
struct nxk_proceso_mensaje { uint32_t origen,valor; };
struct nxk_proceso_info {
    uint32_t id,estado,turnos,cesiones,preempciones,error_motor,dormidas,despertar;
    /* mensajes: cantidad en cola. Los otros cuatro contadores saturan. */
    uint32_t mensajes,enviados,recibidos,esperas_mensaje,plazos;
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
/* FIFO de cuatro mensajes, origen0 para el kernel. No bloquea ni descarta al
 * llenar: 0 exito, -1 ID, -2 lleno, -3 destino terminal o contexto kernel.
 * Solo fuera del ejecutor, con las mismas condiciones que crear/paso. */
int nxk_proceso_enviar(uint32_t destino,uint32_t valor);
/* Puentes internos de INT81: origen/id debe ser el proceso en EJECUTANDO y
 * el gestor debe estar ocupado. Un error no consume ni modifica salida.
 * Recibir: -2 vacio, -1 contrato. Preparar conserva el buffer virtual hasta
 * reanudar; el gestor entrega {origen,valor} antes de retirar de la cola. */
int nxk_proceso_enviar_desde(uint32_t origen,uint32_t destino,uint32_t valor);
int nxk_proceso_recibir_actual(uint32_t id,struct nxk_proceso_mensaje *salida);
int nxk_proceso_preparar_espera(uint32_t id,uint32_t buffer_virtual);
/* Cancelar solo LISTO/DORMIDO/ESPERANDO: limpia buzon y espera, retiene ID y
 * memoria hasta liberar. -1 ID, -2 estado terminal, -3 contexto/gestor ocupado. */
int nxk_proceso_cancelar(uint32_t id);
/* Hasta1024 turnos: 0 sin pendientes, 1 presupuesto, 2 solo dormidos/esperas.
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
