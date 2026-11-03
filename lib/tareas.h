/* Planificador cooperativo propio, una CPU x86 y llamadas desde ring0.
 * Las tareas deben ceder; no llamar desde IRQ ni deshabilitar interrupciones.
 * Los argumentos y su memoria pertenecen al llamador y deben seguir vivos. */
#ifndef NXK_TAREAS_H
#define NXK_TAREAS_H
#include "nxk.h"
#define NXK_TAREAS_MAX 8u
#define NXK_BUZON_MAX 4u
struct nxk_mensaje { uint32_t origen, valor; };
enum nxk_tarea_estado { NXK_TAREA_LIBRE, NXK_TAREA_LISTA, NXK_TAREA_EJECUTANDO,
    NXK_TAREA_DORMIDA, NXK_TAREA_TERMINADA, NXK_TAREA_CANCELADA, NXK_TAREA_ESPERANDO, NXK_TAREA_ESPERANDO_FIN, NXK_TAREA_ESPERANDO_ENVIO };
struct nxk_tarea_info { uint32_t id, estado, activaciones, despertar, resultado, esperando; };
struct nxk_tarea_resultado { uint32_t estado, codigo; };
/* Cero indica exito. IDs cero, liberados o de una generacion vieja son invalidos. */
int nxk_tarea_crear(void (*funcion)(void *),void *argumento,uint32_t *id);
int nxk_tarea_consultar(uint32_t id,struct nxk_tarea_info *info);
/* Enumerar un slot: -2 si esta libre; no expone punteros internos. */
int nxk_tarea_enumerar(unsigned slot,struct nxk_tarea_info *info);
uint32_t nxk_tarea_actual(void); /* Cero fuera de una tarea. */
int nxk_tarea_ceder(void);
int nxk_tarea_dormir(uint32_t ticks); /* 0 equivale a ceder; maximo 2^31-1. */
/* Buzon FIFO no bloqueante: -2 lleno/vacio; -3 destino finalizado.
 * Enviar desde consola kernel usa origen 0. Recibir exige tarea actual.
 * Las variantes con espera evitan sondeo; dormir conserva su plazo. */
int nxk_tarea_enviar(uint32_t destino,uint32_t valor);
/* Solo desde tarea: 0 encolado, 1 plazo/lleno con cero ticks.
 * Conserva valor en su pila hasta tener espacio; no reserva un lugar. */
int nxk_tarea_enviar_esperando(uint32_t destino,uint32_t valor,uint32_t ticks);
int nxk_tarea_recibir(struct nxk_mensaje *mensaje);
/* 0: mensaje; 1: plazo/vacio con cero ticks; -1: contrato invalido.
 * No modifica salida al vencer. Primero atiende mensajes ya disponibles. */
int nxk_tarea_esperar_mensaje(struct nxk_mensaje *mensaje,uint32_t ticks);
/* Finalizar no retorna si tiene exito; fuera de tarea devuelve -1. */
int nxk_tarea_finalizar(uint32_t codigo);
/* 0: terminal; 1: plazo; -1: contrato; -2: ciclo. No consume el resultado.
 * Solo desde tarea. Plazo cero consulta; salida intacta al fallar/vencer. */
int nxk_tarea_esperar_fin(uint32_t id,struct nxk_tarea_resultado *salida,uint32_t ticks);
int nxk_tarea_cancelar(uint32_t id); /* Cualquier espera/lista; no mata la actual. */
int nxk_tarea_liberar(uint32_t id); /* Solo terminales; -3 si quedan esperadores con referencia. */
/* 0: no quedan pendientes; 1: presupuesto agotado, se puede reanudar.
 * Negativos: contrato/IF/pila invalidos. El limite no interrumpe callbacks. */
int nxk_tareas_ejecutar(uint32_t presupuesto_ticks);
int nxk_tareas_demo(void);
int nxk_tareas_mensajes_demo(void);
int nxk_tareas_resultados_demo(void);
int nxk_tareas_demo_cantidad(unsigned cantidad);
int nxk_tareas_probar_limites(void);
/* Lote persistente de demostracion controlado desde la consola. */
int nxk_tareas_preparar_demo(void);
int nxk_tareas_limpiar_demo(void);
#endif
