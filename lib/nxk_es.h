/* GUIA DE LECTURA NXK - lib/nxk_es.h
 * Interfaz entre el codigo generado desde NEXTRY y los servicios del kernel.
 * Contexto y orden de estudio: ver LEER-CODIGO.md en la carpeta principal.
 */
#ifndef NXK_ES_H
#define NXK_ES_H
#include <stdint.h>
void nxk_es_inicio(void);
void nxk_es_agregar_archivo(const char *nombre,const char *texto);
void nxk_es_copiar_archivo(const char *origen,const char *destino);
void nxk_es_renombrar_archivo(const char *nombre,const char *nuevo);
void nxk_es_guardar_archivo(const char *nombre,const char *texto);
void nxk_es_mostrar_archivo(const char *nombre);
void nxk_es_borrar_archivo(const char *nombre);
void nxk_es_mostrar(const char *texto);
void nxk_es_numero(uint32_t valor);
/* Cesion cooperativa: la ruta de ejecucion debe ofrecer reanudacion. */
void nxk_es_ceder(void);
/* Espera cooperativa en ticks, 0 equivale a ceder; limite validado por kernel. */
void nxk_es_dormir(uint32_t ticks);
/* Mensajes de procesos CPL3: FIFO acotada; un error al enviar detiene la app. */
void nxk_es_enviar_mensaje(uint32_t destino,uint32_t valor);
/* 0 entrega origen/valor, 1 vacio o plazo vencido conserva ambas salidas.
 * El plazo maximo es 0x7fffffff ticks; cero consulta sin bloquear. */
uint32_t nxk_es_recibir_mensaje(uint32_t *origen,uint32_t *valor,uint32_t ticks);
uint32_t nxk_es_proceso_actual(void);
/* Mascara de proceso: salida1, leer archivos2, modificar archivos4, enviar8.
 * Solo se pueden retirar bits; nunca recuperarlos durante la misma vida. */
void nxk_es_restringir_permisos(uint32_t mascara);
uint32_t nxk_es_permisos_actuales(void);
/* Cuota de paginas privadas0..4, monotona y nunca inferior al uso. */
void nxk_es_limitar_memoria(uint32_t cuota);
uint32_t nxk_es_cuota_memoria(void);
/* Lista monotona de hasta cuatro nombres. Cantidad0 impide todos los archivos;
 * cualquier error o servicio ausente termina explicitamente la aplicacion. */
void nxk_es_limitar_archivos(uint32_t cantidad,const char *a,const char *b,const char *c,const char *d);
/* Paginas privadas de datos: hasta cuatro, 4096 bytes cada una, solo CPL3.
 * Reserva devuelve direccion o0; liberar devuelve0 o UINT32_MAX.
 * Leer/escribir enteros de32bits requiere direccion alineada dentro de datos.
 * Una pagina no reservada produce fallo contenido en el proceso. */
uint32_t nxk_es_reservar_pagina(void);
uint32_t nxk_es_reservar_paginas(uint32_t cantidad);
uint32_t nxk_es_liberar_paginas(uint32_t direccion,uint32_t cantidad);
/* Escritura0 RO/1 RW, sin cambiar los bytes.0 exito,-1 contrato,-5 IPC.
 * La consulta retorna0 RO/1 RW o-1; solo paginas de datos reservadas. */
uint32_t nxk_es_proteger_paginas(uint32_t direccion,uint32_t cantidad,uint32_t escritura);
uint32_t nxk_es_escritura_pagina(uint32_t direccion);
uint32_t nxk_es_liberar_pagina(uint32_t direccion);
uint32_t nxk_es_leer_entero(uint32_t direccion);
void nxk_es_escribir_entero(uint32_t direccion,uint32_t valor);
/* Buffers protegidos: estados uint32 manejables; -1 se compara UINT32_MAX.
 * Leer/mostrar retorna cantidad, guardar0; recibir/esperar0 entrega u1 plazo.
 * El kernel comprueba permisos y el rango completo antes de efectos. */
uint32_t nxk_es_leer_archivo_en(const char *nombre,uint32_t direccion,uint32_t cantidad,uint32_t desplazamiento);
uint32_t nxk_es_guardar_archivo_desde(const char *nombre,uint32_t direccion,uint32_t cantidad);
uint32_t nxk_es_mostrar_memoria(uint32_t direccion,uint32_t cantidad);
uint32_t nxk_es_recibir_en_memoria(uint32_t direccion,uint32_t ticks);
uint32_t nxk_es_esperar_en_memoria(uint32_t destino,uint32_t direccion,uint32_t ticks);
/* 0 enviado, 1 plazo/cola llena; errores de ID, destino o ciclo detienen la app. */
uint32_t nxk_es_enviar_esperando(uint32_t destino,uint32_t valor,uint32_t ticks);
/* 0 publica estado/codigo terminal; 1 conserva ambas salidas al vencer.
 * Resultado del proceso no consumido; variables de salida distintas. */
uint32_t nxk_es_esperar_proceso(uint32_t destino,uint32_t *estado,uint32_t *codigo,uint32_t ticks);
/* Finaliza el proceso CPL3 con codigo propio; no vuelve a la siguiente orden. */
_Noreturn void nxk_es_finalizar(uint32_t codigo);
uint32_t nxk_es_memoria_usada(void);
uint32_t nxk_es_dividir(uint32_t a, uint32_t b);
uint32_t nxk_es_resto(uint32_t a, uint32_t b);
uint32_t nxk_es_leer_puerto(uint32_t puerto);
void nxk_es_escribir_puerto(uint32_t puerto, uint32_t valor);
_Noreturn void nxk_es_fallo(uint32_t linea);
_Noreturn void nxk_es_detener(void);
#endif
