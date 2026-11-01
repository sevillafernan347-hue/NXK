/* GUIA DE LECTURA NXK - lib/nxk_es.h
 * Interfaz entre el codigo generado desde NEXTRY y los servicios del kernel.
 * Contexto y orden de estudio: ver LEER-CODIGO.md en la carpeta principal.
 */
#ifndef NXK_ES_H
#define NXK_ES_H
#include <stdint.h>
void nxk_es_inicio(void);
void nxk_es_mostrar(const char *texto);
void nxk_es_numero(uint32_t valor);
uint32_t nxk_es_memoria_usada(void);
uint32_t nxk_es_dividir(uint32_t a, uint32_t b);
uint32_t nxk_es_resto(uint32_t a, uint32_t b);
uint32_t nxk_es_leer_puerto(uint32_t puerto);
void nxk_es_escribir_puerto(uint32_t puerto, uint32_t valor);
_Noreturn void nxk_es_fallo(uint32_t linea);
_Noreturn void nxk_es_detener(void);
#endif
