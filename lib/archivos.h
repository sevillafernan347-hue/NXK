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
/* Cambia longitud RAM hasta 256: ampliacion rellena con ceros; no crea archivos. */
int nxk_archivo_truncar(const char *nombre,size_t longitud);
int nxk_archivo_renombrar(const char *nombre,const char *nuevo);
/* Copia RAM/NXFS a una entrada RAM nueva de hasta 256 bytes. */
int nxk_archivo_copiar(const char *origen,const char *destino);
int nxk_archivo_borrar(const char *nombre);
const char *nxk_archivo_nombre(unsigned indice);
/* Instantaneas RAM completas de ocho entradas. Espacios sin usar en cero.
 * Una CPU cooperativa, fuera de IRQ; buffers del llamador no deben superponerse
 * con la tabla interna. Importar reemplaza TODO, solo tras validar la imagen.
 * 0 exito, -1 null, -2 imagen invalida, -3 sin memoria para primera tabla. */
struct nxk_archivo_imagen { char nombre[24]; unsigned char datos[256]; uint32_t longitud; };
int nxk_archivos_exportar(struct nxk_archivo_imagen *salida);
int nxk_archivos_importar(const struct nxk_archivo_imagen *entrada);
int nxk_archivos_probar(void);
#endif
