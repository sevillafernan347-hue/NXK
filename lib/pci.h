/* Inventario PCI legado, mecanismo1, una CPU, llamadas kernel fuera de IRQ.
 * Solo lee configuracion: no habilita DMA, BARs ni dispositivos. */
#ifndef NXK_PCI_H
#define NXK_PCI_H
#include "nxk.h"
#define NXK_PCI_MAX 64u
struct nxk_pci_dispositivo {
    uint32_t bus, ranura, funcion, fabricante, dispositivo,
        clase, subclase, interfaz, revision, cabecera, irq;
};
struct nxk_pci_resumen { uint32_t detectados, guardados, omitidos; };
/* Scan sustituye inventario sin heap; >64 reporta omitidos, nunca desborda. */
int nxk_pci_explorar(void);
int nxk_pci_resumen(struct nxk_pci_resumen *salida);
int nxk_pci_consultar(unsigned indice,struct nxk_pci_dispositivo *salida);
/* N-esimo dispositivo de clase 0..255 del ultimo inventario; -2 si no existe. */
int nxk_pci_buscar_clase(uint32_t clase,unsigned indice,struct nxk_pci_dispositivo *salida);
/* DWORD alineado 0..252, bus0..255, ranura0..31, funcion0..7.
 * -1 contrato; ausencia devuelve DWORD con fabricante 0xffff en registro0. */
int nxk_pci_leer32(unsigned bus,unsigned ranura,unsigned funcion,unsigned registro,uint32_t *salida);
#ifdef NXK_TEST
int nxk_pci_probar(void);
#endif
#endif
