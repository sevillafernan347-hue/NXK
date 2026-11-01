/* GUIA DE LECTURA NXK - lib/nxe.h
 * Contrato binario de servicios NXE: version y punteros a funciones que recibe una app nativa.
 * Contexto y orden de estudio: ver LEER-CODIGO.md en la carpeta principal.
 */
#ifndef NXE_H
#define NXE_H
#include "nxk_es.h"
#include <stddef.h>
/* ABI NXE 1. La disposicion de punteros depende de la arquitectura del archivo. */
typedef struct {
    uint32_t version;
    uint32_t longitud;
    void (*mostrar)(const char *);
    void (*numero)(uint32_t);
    uint32_t (*memoria_usada)(void);
    uint32_t (*dividir)(uint32_t, uint32_t);
    uint32_t (*resto)(uint32_t, uint32_t);
    uint32_t (*leer_puerto)(uint32_t);
    void (*escribir_puerto)(uint32_t, uint32_t);
    void (*fallo)(uint32_t);
    void (*detener)(void);
} NXE_API;
int nxk_nxe_cargar(const unsigned char *datos, size_t longitud);
int nxk_nxe_demo(void);
#endif
