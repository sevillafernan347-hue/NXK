/* ATA PIO LBA28, canal IDE primario. Una operacion por vez, sin concurrencia. */
#ifndef NXK_ATA_H
#define NXK_ATA_H
#include "nxk.h"
struct nxk_ata_info { uint32_t sectores; char modelo[41]; };
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
