/* ATA PIO LBA28, canal IDE primario. Sin escritura ni concurrencia. */
#ifndef NXK_ATA_H
#define NXK_ATA_H
#include "nxk.h"
struct nxk_ata_info { uint32_t sectores; char modelo[41]; };
int nxk_ata_identificar(unsigned unidad,struct nxk_ata_info *info);
/* Requiere identificar; destino de al menos 512 bytes, solo punteros kernel. */
int nxk_ata_leer_sector(unsigned unidad,uint32_t lba,void *destino);
int nxk_ata_probar(void);
#endif
