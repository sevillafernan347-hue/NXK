/* NXFS1 de solo lectura: IDE primario esclavo, maximo 8 archivos de 4096 bytes. */
#ifndef NXK_VOLUMEN_H
#define NXK_VOLUMEN_H
#include "nxk.h"
int nxk_volumen_montar(void);
/* Sin I/O: -1 si esta desmontado, 0..8 archivos si esta montado. */
int nxk_volumen_estado(void);
/* Invalida el catalogo y la lectura secuencial sin tocar el disco. */
void nxk_volumen_desmontar(void);
/* Nombre prestado hasta el siguiente montaje, desmontaje o reinicio ATA. */
const char *nxk_volumen_nombre(unsigned indice);
enum {
    NXK_VOLUMEN_SIN_ENTRADA=-2,
    NXK_VOLUMEN_SIN_MEMORIA=-4,
    NXK_VOLUMEN_ERROR_ATA=-5,
    NXK_VOLUMEN_HUELLA_INVALIDA=-6
};
/* Verifica la entrada directamente desde NXFS. Devuelve 0 o un error de arriba.
 * Comparte el buffer de lectura y no entrega el contenido al llamador. */
int nxk_volumen_verificar(unsigned indice);
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

/* Verificacion por sectores, solo contexto kernel serial x86.
 * Un resultado terminal conserva su progreso hasta el siguiente inicio. */
enum nxk_volumen_tarea_estado {
    NXK_VOLUMEN_TAREA_INACTIVA,
    NXK_VOLUMEN_TAREA_PENDIENTE,
    NXK_VOLUMEN_TAREA_VERIFICADA,
    NXK_VOLUMEN_TAREA_CANCELADA=-7,
    NXK_VOLUMEN_TAREA_INVALIDADA=-8
};
struct nxk_volumen_tarea {
    uint32_t leidos,total;
    int estado;
};
/* 0 admitida, -1 nombre nulo, -2 no existe, -3 ya hay tarea,
 * -8 revision de disco agotada. Rechazar no reemplaza un resultado previo. */
int nxk_volumen_tarea_iniciar(const char *nombre);
void nxk_volumen_tarea_paso(void);
void nxk_volumen_tarea_cancelar(void);
struct nxk_volumen_tarea nxk_volumen_tarea_consultar(void);

/* Descriptor kernel copiado al suspender. La revision lo liga al montaje. */
struct nxk_volumen_lectura {
    uint32_t lba,longitud,huella,revision;
};
#define NXK_VOLUMEN_LECTURA_PENDIENTE 257
int nxk_volumen_lectura_preparar(const char *nombre,struct nxk_volumen_lectura *salida);
/* Hasta un paso ATA. Pendiente no escribe en salida, igual que un error.
 * Solo el planificador x86 puede usar un ID y un destino kernel. */
int nxk_volumen_lectura_paso(uint32_t id,const struct nxk_volumen_lectura *archivo,
                            uint32_t posicion,uint32_t capacidad,void *salida);
void nxk_volumen_lectura_olvidar(uint32_t id);
#endif
