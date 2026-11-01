/* Archivos volatiles propios: nombres de 1..23 caracteres, 8 entradas. */
#ifndef NXK_ARCHIVOS_H
#define NXK_ARCHIVOS_H
#include "nxk.h"
int nxk_archivo_escribir(const char *nombre,const void *datos,size_t n);
int nxk_archivo_leer(const char *nombre,void *datos,size_t capacidad);
int nxk_archivo_borrar(const char *nombre);
const char *nxk_archivo_nombre(unsigned indice);
int nxk_archivos_probar(void);
#endif
