# Inspeccion estatica de ntoskrnl.exe

Archivo aportado: 13131136 bytes.
SHA256: 4a6af9a3dbbeb440c4a9fbe8a4d6832eae751ac3a85860d860b8313a737a2e3a
Maquina: 0x8664 (AMD64). Formato: 0x20b (PE32+).
Subsistema: 1 (nativo). Secciones: 36.
Se examinaron cabeceras y tabla de secciones sin ejecutar el binario.
No se verifico firma Authenticode, origen ni autenticidad. No es codigo fuente.
No se copio ni redistribuyo el binario en NXK.

## Mejora propia resultante

El cargador PE32 de NXK ahora exige que el punto de entrada este respaldado
por bytes reales de una seccion ejecutable. Antes aceptaba tambien la zona
virtual rellenada con ceros. Se agregaron pruebas sinteticas de rechazo para
esa entrada, formato PE32+ y subsistema nativo. Las pruebas no ejecutan el
archivo aportado. Matriz x86 32/64/128/1024 MiB superada y demo PE32 conservada.
Esto no incorpora compatibilidad con el kernel Windows, controladores o apps
Windows generales. El cargador sigue reservado a demostraciones confiables
ring0; no ofrece aislamiento ni permisos de memoria por seccion.

## Secciones observadas

| Nombre | RVA | Tamano virtual | Bytes en archivo |
| --- | --- | --- | --- |
| .rdata | 0x1000 | 916128 | 917504 |
| .pdata | 0xe1000 | 455940 | 458752 |
| .idata | 0x151000 | 10620 | 12288 |
| .edata | 0x154000 | 111942 | 114688 |
| PROTDATA | 0x170000 | 1 | 4096 |
| GFIDS | 0x171000 | 43840 | 45056 |
| Pad1 | 0x17c000 | 540672 | 0 |
| .text | 0x200000 | 5229695 | 5230592 |
| PAGE | 0x6fd000 | 4575424 | 4579328 |
| PAGELK | 0xb5b000 | 156956 | 159744 |
| POOLCODE | 0xb82000 | 9513 | 12288 |
| PAGEKD | 0xb85000 | 24042 | 24576 |
| PAGEVRFY | 0xb8b000 | 202009 | 204800 |
| PAGEHDLS | 0xbbd000 | 9079 | 12288 |
| PAGEBGFX | 0xbc0000 | 26980 | 28672 |
| TRACESUP | 0xbc7000 | 6563 | 8192 |
| PAGECMRC | 0xbc9000 | 4173 | 8192 |
| KVASCODE | 0xbcb000 | 9368 | 12288 |
| KSCP | 0xbce000 | 2112 | 4096 |
| DRVPRX | 0xbcf000 | 183 | 4096 |
| fothk | 0xbd0000 | 4096 | 4096 |
| INITKDBG | 0xbd1000 | 105526 | 106496 |
| MINIEX | 0xbeb000 | 9916 | 12288 |
| INIT | 0xbee000 | 669416 | 671744 |
| Pad2 | 0xc92000 | 1499136 | 0 |
| .data | 0xe00000 | 1855184 | 65536 |
| ALMOSTRO | 0xfc5000 | 40064 | 8192 |
| CACHEALI | 0xfcf000 | 36224 | 4096 |
| PAGEDATA | 0xfd8000 | 111760 | 8192 |
| PAGEVRFD | 0xff4000 | 80976 | 40960 |
| INITDATA | 0x1008000 | 136372 | 4096 |
| Pad3 | 0x102a000 | 1925120 | 0 |
| CFGRO | 0x1200000 | 8256 | 12288 |
| Pad4 | 0x1203000 | 2084864 | 0 |
| .rsrc | 0x1400000 | 232700 | 233472 |
| .reloc | 0x1439000 | 91772 | 94208 |
