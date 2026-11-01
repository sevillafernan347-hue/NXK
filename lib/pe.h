/* GUIA DE LECTURA NXK - lib/pe.h
 * Interfaz publica del cargador experimental de programas Windows PE32.
 * Contexto y orden de estudio: ver LEER-CODIGO.md en la carpeta principal.
 */
#ifndef NXK_PE_H
#define NXK_PE_H
#include <stddef.h>
#include <stdint.h>
int nxk_pe_cargar(const unsigned char *datos, size_t longitud);
int nxk_pe_demo(void);
void nxk_pe_salida(const char *datos, uint32_t longitud);
#endif
