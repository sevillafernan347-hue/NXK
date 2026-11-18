/* NXFS1 de solo lectura: IDE primario esclavo, maximo 8 archivos de 4096 bytes. */
#ifndef NXK_VOLUMEN_H
#define NXK_VOLUMEN_H
#include "nxk.h"
int nxk_volumen_montar(void);
const char *nxk_volumen_nombre(unsigned indice);
/* Tamano registrado al montar. Sin I/O: no verifica el contenido.
 * 0 exito, -1 puntero nulo, -2 nombre no registrado. */
int nxk_volumen_tamano(const char *nombre,uint32_t *salida);
/* Tramos contiguos pueden compartir una copia verificada. Offset cero,
 * otra verificacion NXFS o una nueva revision del esclavo inician otra copia. */
int nxk_volumen_leer_parte(const char *nombre,void *destino,size_t capacidad,uint32_t desplazamiento);
int nxk_volumen_leer(const char *nombre,void *destino,size_t capacidad);
/* Visita una vez tras verificar todo el archivo. Vacio: puntero nulo, longitud 0.
 * Buffer prestado solo durante la llamada. El visitante no debe ceder ni
 * llamar al volumen. Devuelve 0 al visitar o el error de lectura equivalente. */
int nxk_volumen_visitar(const char *nombre,void (*visitar)(const unsigned char *,size_t));
#endif
