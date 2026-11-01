/* NXFS1 de solo lectura: IDE primario esclavo, maximo 8 archivos de 4096 bytes. */
#ifndef NXK_VOLUMEN_H
#define NXK_VOLUMEN_H
#include "nxk.h"
int nxk_volumen_montar(void);
const char *nxk_volumen_nombre(unsigned indice);
int nxk_volumen_leer_parte(const char *nombre,void *destino,size_t capacidad,uint32_t desplazamiento);
int nxk_volumen_leer(const char *nombre,void *destino,size_t capacidad);
#endif
