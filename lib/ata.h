/* ATA PIO LBA28, canal IDE primario. Una operacion por vez, sin concurrencia. */
#ifndef NXK_ATA_H
#define NXK_ATA_H
#include "nxk.h"
struct nxk_ata_info { uint32_t sectores; char modelo[41]; };
enum nxk_ata_limite {
    NXK_ATA_LIMITE_NINGUNO,
    NXK_ATA_LIMITE_PIT,
    NXK_ATA_LIMITE_SONDEOS
};
/* Ultima operacion valida por unidad; consulta solo en contexto kernel serial.
 * comando: EC identificar,20 leer,30 escribir,E7 sincronizar (hexadecimal).
 * fase: 0 sin operacion,1 antes del comando,2 espera de datos,3 cierre.
 * emitido distingue fallo previo de fallo del comando solicitado.
 * error solo es interpretable cuando error_valido=1; sondeos no mide tiempo.
 * limite indica la causa de -4 y es cero en cualquier otro resultado. */
struct nxk_ata_diagnostico {
    uint32_t comando,lba,fase,emitido,estado,error,error_valido,sondeos;
    int32_t resultado;
    uint32_t limite;
};
/* Sin I/O ni asignaciones. -1 argumento invalido; -64 en x64; salida intacta. */
int nxk_ata_consultar(unsigned unidad,struct nxk_ata_diagnostico *salida);
/* Revision del esclavo sin I/O. Cambia antes de identificarlo o escribir.
 * UINT32_MAX indica que el llamador no puede reutilizar lecturas antiguas. */
uint32_t nxk_ata_revision_esclavo(void);
/* Errores: -1 argumento/capacidad; -2 ausente; -3 ERR/DF; -4 espera agotada;
 * -5 IDENTIFY incompatible; -64 plataforma sin soporte. Identificar/leer
 * conservan la salida del llamador al fallar. Identificar invalida primero
 * la capacidad anterior; solo publica la nueva al completar la operacion.
 * En fase previa, ERR residual por si solo no impide un nuevo comando. */
int nxk_ata_identificar(unsigned unidad,struct nxk_ata_info *info);
/* Requiere identificar; destino de al menos 512 bytes, solo punteros kernel. */
int nxk_ata_leer_sector(unsigned unidad,uint32_t lba,void *destino);
/* Solo esclavo 1 identificado; origen de 512 bytes, puede estar desalineado.
 * El llamador debe validar que es un volumen propio escribible. Exito aun puede
 * estar en cache; sincronizar debe completar antes de anunciar persistencia. */
int nxk_ata_escribir_sector(unsigned unidad,uint32_t lba,const void *origen);
/* FLUSH CACHE en esclavo; rechaza maestro, unidad no identificada y errores. */
int nxk_ata_sincronizar(unsigned unidad);
int nxk_ata_probar(void);
#endif
