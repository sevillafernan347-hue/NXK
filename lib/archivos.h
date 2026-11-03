/* Archivos volatiles propios: nombres de 1..23 caracteres, 8 entradas. */
#ifndef NXK_ARCHIVOS_H
#define NXK_ARCHIVOS_H
#include "nxk.h"
int nxk_archivo_escribir(const char *nombre,const void *datos,size_t n);
int nxk_archivo_leer(const char *nombre,void *datos,size_t capacidad);
/* Lee RAM primero; solo si falta busca el volumen NXFS montado. */
int nxk_archivo_leer_parte(const char *nombre,void *datos,size_t capacidad,uint32_t desplazamiento);
int nxk_archivo_leer_visible(const char *nombre,void *datos,size_t capacidad);
/* Agrega solo a archivos existentes; rechaza desbordamiento sin modificarlos. */
int nxk_archivo_agregar(const char *nombre,const void *datos,size_t n);
int nxk_archivo_renombrar(const char *nombre,const char *nuevo);
/* Copia RAM/NXFS a una entrada RAM nueva de hasta 256 bytes. */
int nxk_archivo_copiar(const char *origen,const char *destino);
int nxk_archivo_borrar(const char *nombre);
const char *nxk_archivo_nombre(unsigned indice);
int nxk_archivos_probar(void);
#endif
